const dgram = require("dgram");
const path = require("path");
const { fileURLToPath } = require("url");
const { processBindFile } = require("./parser.js");

const server = dgram.createSocket("udp4");
const __filename = fileURLToPath(import.meta.url);
server.bind(53);

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
