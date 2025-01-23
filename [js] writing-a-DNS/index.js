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

  // extracting domain queries, query type, records from the DNS Zone file
  let [recordsResult, qt, domainParts, askedRecord] = await getRecords(
    msg.slice(12) // all request bytes, without header
  );

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

// prepare flags for DNS response
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
        break; // end of label
      }
    } else {
      state = 1;
      expectedLength = byte;
    }
    y++;
  }

  // Qtype
  let qType = data.slice(y, y + 2);
  // we assume that qClass = In (standard for the internet)
  return [domainParts, qType];
}

// ---- GET ASKED RECORDS ----
async function getRecords(data) {
  let [domain, qType] = getQuestion(data);
  let askedRecord = "@";
  let domainName;

  // costructing domain name from labels
  if (domain.length > 2) {
    askedRecord = domain[0]; // eg: www
    domainName = domain[1] + "." + domain[2]; // eg: example.com
  } else {
    domainName = domain.join("."); // when 'www' is omitted
  }

  let recordType = getRecordType(qType);
  let filePath = path.join(path.dirname(__filename), `zones/${domain}.zone`);

  let records = await processBindFile(filePath);
  return [records, recordType, domainName, askedRecord];
}

// -------- QTYPE LOOK UP TABLES --------
// lookup table: binary Qtype -> string Qtype
function getRecordType(binary_qType) {
  let qt;
  let hexedType = binary_qType.toString("hex");
  switch (hexedType) {
    case "0001":
      qt = "A";
      break;
    case "0002":
      qt = "NS";
      break;
    case "0005":
      qt = "CNAME";
      break;
    case "0006":
      qt = "SOA";
      break;
    case "000c":
      qt = "PTR";
      break;
    case "000f":
      qt = "MX";
      break;
    case "0010":
      qt = "TXT";
      break;
    default:
      break;
  }

  return qt;
}

// lookup table:  string Qtype -> binary Qtype
function getRecordTypeHex(string_Qtype) {
  let recordTypeHex;
  switch (string_Qtype) {
    case "A":
      recordTypeHex = "0001";
      break;
    case "NS":
      recordTypeHex = "0002";
      break;
    case "CNAME":
      recordTypeHex = "0005";
      break;
    case "SOA":
      recordTypeHex = "0006";
      break;
    case "PTR":
      recordTypeHex = "000c";
      break;
    case "MX":
      recordTypeHex = "000f";
      break;
    case "TXT":
      recordTypeHex = "0010";
      break;
    default:
      break;
  }
  return recordTypeHex;
}
