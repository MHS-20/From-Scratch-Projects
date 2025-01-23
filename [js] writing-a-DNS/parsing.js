const fs = require("fs");
const readline = require("readline");

async function processBindFile(filePath = "zones/example2.com.zone") {
  // creating readline interface
  const fileStream = fs.createReadStream(filePath);
  const rl = readline.createInterface({
    input: fileStream,
    crlfDelay: Infinity,
  });

  // parsing variables
  let origin;
  let ttl;
  let previousTtl;
  let previousName;

  let records = [];
  let recordsType = ["NS", "A", "CNAME", "MX"];

  recordsType.forEach((element) => {
    records[element] = [];
  });

  let soa = {};
  let soaLine = 0;
  let multiLineSoa = false;
  let containsTtl = false;

  // check comment
  // await
  for (const line of rl) {
    if (line.length > 1) {
      line = line.trim().replace(/\t/g, " ").replace(/\s+/g, " ");

      let commentLine = false;
      let commentIndex = line.indexOf(";");

      if (commentIndex != -1) {
        if (commentIndex != 0) {
          let commentSplit = line.split(";");
          line = commentSplit[0];
        } else {
          commentLine = true;
        }
      }

      // parse basic cases
      if (!commentLine) {
        let lineSplit = line.split(" ");
        switch (lineSplit[0]) {
          case "$ORIGIN":
            origin = lineSplit[1];
            break;
          case "$TTL":
            ttl = lineSplit[1];
            break;
          case "$INCLUDE":
            break; // not supported
          default:
            if (lineSplit.includes("SOA")) {
              previousName = lineSplit[0];
              soa.mname = lineSplit[3];
              soa.rname = lineSplit[4];

              if (lineSplit.includes(")")) {
                soa.serial = lineSplit[6];
                soa.refresh = lineSplit[7];
                soa.retry = lineSplit[8];
                soa.expire = lineSplit[9];
                soa.title = lineSplit[10];
              } else {
                multiLineSoa = true;
                soaLine++;
              }
            }
        }

        // parse normal records
        recordsType.forEach((element) => {
          if (lineSplit.includes(element)) {
            let type = element;
            let rr;

            [rr, previousName, previousTtl] = processRr(
              splittedLine,
              containsTtl,
              previousTtl,
              previousName,
              origin,
              ttl
            );

            records[type].push(rr);
          }
        });

        // parse multiline SOA case
        if (multiLineSoa) {
          switch (soaLine) {
            case 2:
              soa.serial = lineSplit[0];
              break;
            case 3:
              soa.refresh = lineSplit[0];
              break;
            case 4:
              soa.retry = lineSplit[0];
              break;
            case 5:
              soa.expire = lineSplit[0];
              break;
            case 6:
              soa.ttl = lineSplit[0];
              break;
            default:
              break;
          }
          if (lineSplit.includes(")")) {
            multiLineSoa = false;
          }
          soaLine++;
        }
      } // if !commentLine
    } //end: if line > 1
  } // end: for

  return records;
}


// process resource record
function processRr(splittedLine, containsTtl, previousTtl, previousName, origin, ttl) {
  const rr = {};
  const totalLength = splittedLine.length;
  const isMx = !isNaN(Number(splittedLine[totalLength - 2]));
  
  // Helper function to check and extract TTL
  const extractTTL = () => {
    for (let index = 0; index < splittedLine.length; index++) {
      const element = splittedLine[index];
      if (!element.includes(".") && !isNaN(parseInt(element))) {
        containsTtl = true;
        previousTtl = element;
        splittedLine.splice(index, 1);
        break;
      }
    }
  };

  // Process resource records based on line length
  switch (totalLength) {
    case 5: // same as 4
    case 4:
      extractTTL();

      if (containsTtl) {
        rr.class = splittedLine[0];
        rr.type = splittedLine[1];
        rr.data = splittedLine[2];

      // MX record with explicit name
      } else if (isMx && totalLength === 4) {
        previousName = "@";
        rr.class = splittedLine[0];
        rr.type = splittedLine[1];
        rr.preference = splittedLine[2];
        rr.data = splittedLine[3];

      // Regular case with explicit name
      } else {
        previousName = splittedLine[0];
        rr.class = splittedLine[1];
        rr.type = splittedLine[2];
        rr.data = splittedLine[3];
      }
      break;

    case 3:
      // Compact syntax with no TTL or explicit name
      rr.class = splittedLine[0];
      rr.type = splittedLine[1];
      rr.data = splittedLine[2];
      break;

    case 2:
      // No meaningful processing for lines with only 2 parts
      break;

    default:
      // Handle here unexpected cases gracefully
      break;
  }

  rr.name = previousName || origin;
  rr.ttl = previousTtl || ttl;
  return [rr, previousName, previousTtl];
}


export { processBindFile };
