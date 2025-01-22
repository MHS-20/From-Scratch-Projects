import fs from "fs";
import readline from "readline";

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
          if (lineSplit.includes( ")" )) {
            multiLineSoa = false;
          }
          soaLine++;
        }
      } // if !commentLine
    } //end: if line > 1
  } // end: for

  return records; 
}