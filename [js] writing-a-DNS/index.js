const dgram = require("dgram");
const path = require("path");
const { fileURLToPath } = require("url");
const { processBindFile } = require("./parser.js");

const server = dgram.createSocket("udp4");
const __filename = fileURLToPath(import.meta.url);
server.bind(53);

//------ MAIN MESSAGE HANDLER --------
server.on("message", async (msg, rinfo) => {
  let ID = msg.slice(0, 2); // first two bytes
  let FLAGS = getFlags(msg.slice(2, 4)); // flag bytes for response

  // convert to hex
  FLAGS = new Buffer.from(parseInt(FLAGS, 2).toString(16), "hex");

  // assuming only one question (for simplicity)
  let QDCOUNT = new Buffer.from("0001", "hex");

  let [recordsResult, qt, domainParts, askedRecord] = await getRecords(
    msg.slice(12) // all request bytes, without header
  );

  //  extracting domain queries, query type, records from the DNS Zone file
  let askedRecords = recordsResult[qt].filter((el) => el.name == askedRecord);
  let ANCOUNT = askedRecords.length.toString(16).padStart(4, 0);

  ANCOUNT = new Buffer.from(ANCOUNT, "hex");
  let NSCOUNT = new Buffer.from("0000", "hex");
  let ARCOUNT = new Buffer.from("0000", "hex");
  let domainQuestion = new Buffer.from(buildQuestion(domainParts, qt), "hex");
  let dnsBody = "";

  for (let record of askedRecords) {
    dnsBody += recordToBytes(qt, record);
  }

  dnsBody = new Buffer.from(dnsBody, "hex");
  server.send(
    [ID, FLAGS, QDCOUNT, ANCOUNT, NSCOUNT, ARCOUNT, domainQuestion, dnsBody],
    rinfo.port
  );
});

// ------- GET FLAGS ---------
function extractOPcode(data) {
  let opcode = "";
  for (let bit = 1; bit < 5; bit++) {
    opcode += (data.toString().charCodeAt() & (1 << bit)).toString();
  }
  return opcode;
}

function getFlags(flags) {
  let QR = "1";
  let AA = "1";
  let TC = "0";
  let RD = "0";

  let byte1 = flags.slice(0, 1);
  let OPCode = extractOPcode(byte1);

  // Byte 2
  let RA = "0";
  let Z = "000";
  let RCODE = "0000";
  let header1 = QR + OPCode + AA + TC + RD;
  let header2 = RA + Z + RCODE;

  return header1 + header2;
}

// --- PARSE REQUEST SECTION ---
// Qname, Qtype, Qclass
function getQuestion(data) {
    let state = 0;
    let expectedLength = 0;
    let domainString = "";
    let domainParts = [];
    let x = 0; // length counter for each label
    let y = 0; // length counter for the domain name
  
    // extract labels, each one is prefixed by its length
    // eg: [3]www[7]example[3]com[0]
  
    for (const byte of data) {
      if (state == 1) {
        domainString += String.fromCharCode(byte);
        x++;
  
        if (x == expectedLength) {
          domainParts.push(domainString);
          domainString = "";
          state = 0;
          x = 0;
        }
  
        if (byte == 0) {
          break;  // end of label
        }
  
      } else {
        state = 1;
        expectedLength = byte;
      }
  
      y++;
    } // end for
  
    // Qtype
    let recordType = data.slice(y, y + 2);
  
    // we assume that qClass = In (standard for the internet)
    return [domainParts, recordType];
  }
  

