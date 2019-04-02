// app.js
// JS file.
// By: Hrutvikkumar Patel (hrutvikk@uoguelph.ca)
// Student ID: 1002517
// Date: March 22, 2019
// CIS2750
// Assignment #3

'use strict'

// C library API
const ffi = require('ffi');
const ref = require('ref');
const mysql = require('mysql');

// Express App (Routes)
const express = require("express");
const app = express();
const path = require("path");
const fileUpload = require('express-fileupload');

app.use(fileUpload());

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Send HTML at root, do not change
app.get('/', function (req, res) {
	res.sendFile(path.join(__dirname + '/public/index.html'));
});

// Send Style, do not change
app.get('/style.css', function (req, res) {
	//Feel free to change the contents of style.css to prettify your Web app
	res.sendFile(path.join(__dirname + '/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js', function (req, res) {
	fs.readFile(path.join(__dirname + '/public/index.js'), 'utf8', function (err, contents) {
		const minimizedContents = JavaScriptObfuscator.obfuscate(contents, { compact: true, controlFlowFlattening: true });
		res.contentType('application/javascript');
		res.send(minimizedContents._obfuscatedCode);
	});
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function (req, res) {
	if (!req.files) {
		return res.status(400).send('No files were uploaded.');
	}

	let uploadFile = req.files.uploadFile;
	// check if file doesn't already exist in uploads dir, if it does return error
	if (!fs.existsSync('uploads/' + uploadFile.name)) {

		// Use the mv() method to place the file somewhere on your server
		uploadFile.mv('uploads/' + uploadFile.name, function (err) {
			if (err) {
				return res.status(500).send(err);
			}

			// if no error in moving the file, check if the file is valid
			var filename = uploadFile.name;
			var path = 'uploads/' + uploadFile.name;
			var status = sharedLib.sharedValidateUploadedFile(path);

			if (status != 0) { // if status is not OK send error message
				fs.unlinkSync(path);
				return res.status(500).send(filename + " is an invalid ICalendar file. Upload stopped.")
			}

			// if every thing is code send OK message
			return res.status(200).send(uploadFile.name + " file has been uploaded.")
		});
	}
	else { // send if file already exists in dir
		return res.status(500).send(uploadFile.name + " file already exists in uploads directory.");
	}

});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function (req, res) {
	fs.stat('uploads/' + req.params.name, function (err, stat) {
		console.log(err);
		if (err == null) {
			res.sendFile(path.join(__dirname + '/uploads/' + req.params.name));
		} else {
			res.send('');
		}
	});
});

//******************** Your code goes here ******************** 

// watch for changes to upload dir

// Creating a new object called sharedLib that consist of the c function definitions in js
var cal = ref.types.void;
var calPtr = ref.refType(cal);
var calPtrPtr = ref.refType(calPtr);

let sharedLib = ffi.Library('./libcal', {
	'createCalendar': ['int', ['string', calPtrPtr]],
	'calendarToJSON': ['string', [calPtr]],
	'sharedAddEventToCalendar': ['int', ['string', 'string', 'string', 'string', 'string']],
	'sharedCreateCalendar': ['int', ['string', 'string', 'string', 'string', 'string', 'string']],
	'sharedValidateUploadedFile': ['int', ['string']],
	'printError': ['string', ['int']]
});


// gets all files from server
app.get('/getAllFiles', function (req, res) {
	fs.readdir(__dirname + '/uploads', function (error, filenames) {
		if (error == null) {

			let myJSON = [];
			var i = 0;
			for (i in filenames) {
				// create JSON for all files and save it into an array of calendar JSON
				let dbPtrPtr = ref.alloc(calPtrPtr);
				let status = sharedLib.createCalendar("./uploads/" + filenames[i], dbPtrPtr);
				let fileCalPtr = dbPtrPtr.deref();
				let fileJSON = sharedLib.calendarToJSON(fileCalPtr);
				let newJSON = {
					"status": status,
					"filename": filenames[i],
					"calToJSON": JSON.parse(fileJSON)
				};
				myJSON.push(newJSON);
			}
			// send data
			res.send(myJSON);
		}
		else {
			res.send("Error:" + error);
		}
	});
});

// get single file from server
app.get('/getSingleFile', function (req, res) {
	let myJSON = [];
	// get JSON of file
	let dbPtrPtr = ref.alloc(calPtrPtr);
	let status = sharedLib.createCalendar("./uploads/" + req.query.filename, dbPtrPtr);
	let fileCalPtr = dbPtrPtr.deref();
	let fileJSON = sharedLib.calendarToJSON(fileCalPtr);
	let newJSON = {
		"status": status,
		"filename": req.query.filename,
		"calToJSON": JSON.parse(fileJSON)
	};
	myJSON.push(newJSON);

	// if creating the JSON is ok then send data, else end error
	if (status == 0) {
		res.send(myJSON);
	}
	else {
		res.send('');
	}

});

// add event to a calendar 
app.get('/addEvent', function (req, res) {
	var data = req.query;
	var filename = data.filename;

	// build all the json objects
	var UID = {
		UID: data.UID
	}
	// optioan property summary
	var summary = {
		propName: "summary",
		propDescr: data.summary
	};

	// date time JSON objects
	var startDateTime = {
		date: data.startDate.replace(/-/g, ""),
		time: data.startTime.replace(/:/g, "") + "00",
		UTC: data.startUTC === "on" ? true : false
	};

	var creationDateTime = {
		date: data.creationDate.replace(/-/g, ""),
		time: data.creationTime.replace(/:/g, "") + "00",
		UTC: data.creationUTC === "on" ? true : false
	};

	// add the event to calendar and get error message from wrapper function
	var ICalErrorCode = sharedLib.sharedAddEventToCalendar("./uploads/" + filename, JSON.stringify(creationDateTime),
		JSON.stringify(startDateTime), JSON.stringify(UID), JSON.stringify(summary));

	// get readable version of error message
	var error = sharedLib.printError(ICalErrorCode);
	var message;

	// if OK send good message else send error message
	if (ICalErrorCode == 0) {
		message = "Added a new event to " + filename;
	}
	else {
		message = "The process creating, validating and writing the new iCalendar was unsuccessful. " + filename + " was corrupted due to " + error + ".";
	}

	// data to send back contains status of request, status as number, and readible error message message
	var err = {
		status: ICalErrorCode,
		error: error,
		message: message
	};

	res.send(err);
});

// create calendar
app.get('/createCalendar', function (req, res) {
	var data = req.query;
	var filename = data.fileprefix + ".ics";

	// build all the json objects
	var cal = {
		version: parseFloat(data.version),
		prodID: data.prodID
	}

	var UID = {
		UID: data.UID
	}

	// optioan property summary
	var summary = {
		propName: "summary",
		propDescr: data.summary
	};

	// date time JSON objects
	var startDateTime = {
		date: data.startDate.replace(/-/g, ""),
		time: data.startTime.replace(/:/g, "") + "00",
		UTC: data.startUTC === "on" ? true : false
	};

	var creationDateTime = {
		date: data.creationDate.replace(/-/g, ""),
		time: data.creationTime.replace(/:/g, "") + "00",
		UTC: data.creationUTC === "on" ? true : false
	};

	// create new calendar and get error message from wrapper function	
	var ICalErrorCode = sharedLib.sharedCreateCalendar("./uploads/" + filename, JSON.stringify(cal), JSON.stringify(creationDateTime),
		JSON.stringify(startDateTime), JSON.stringify(UID), JSON.stringify(summary));

	var error = sharedLib.printError(ICalErrorCode);
	var message;

	// if OK send good message else send error message
	if (ICalErrorCode == 0) {
		message = "Created new iCalendar " + filename;
	}
	else {
		message = "The process creating, validating and writing the new iCalendar was unsuccessful. " + filename + " was corrupted due to " + error + ".";
	}

	// data to send back contains status of request, status as number, and readible error message message
	var err = {
		status: ICalErrorCode,
		error: error,
		message: message
	};

	res.send(err);
});


/******** Datebase ********/

// connection

function getErrorMessage(status, message) {
	var error = {
		status: status,
		message: message
	};
	return error;
}

// connection global variable
var connection;

// login/connect to Database.
app.get('/connectDatabase', function (req, res) {
	var data = req.query;

	connection = mysql.createConnection({
		host: 'dursley.socs.uoguelph.ca',
		user: data.username,
		password: data.password,
		database: data.database
	});

	connection.connect(function (err) {
		// If error reprompt user for credentials
		if (err) {
			res.send(getErrorMessage(-1, "Error Connecting! Please try again."));
		}
		// else success and create tables if needed.
		else {
			res.send(getErrorMessage(1, "Connected to database: " + data.database));
			tablesToCreate();
		}
	});
});

// logouts out user and disconnects from database
app.get('/disconnectDatabase', function (req, res) {
	connection.end();
	res.send(getErrorMessage(1, "Disconnected database and logged out."));
});

// creates tables for user if they do not exist
function tablesToCreate() {
	var fileTable = "CREATE TABLE FILE ("
		+ "cal_id INT AUTO_INCREMENT PRIMARY KEY,"
		+ "file_Name VARCHAR(60) NOT NULL,"
		+ "version INT NOT NULL,"
		+ "prod_id VARCHAR(256) NOT NULL)";

	createTable(fileTable);

	var eventTable = "CREATE TABLE EVENT ("
		+ "event_id INT AUTO_INCREMENT PRIMARY KEY,"
		+ "summary VARCHAR(1024),"
		+ "start_time DATETIME NOT NULL,"
		+ "location VARCHAR(60),"
		+ "organizer VARCHAR(256),"
		+ "cal_file INT NOT NULL,"
		+ "FOREIGN KEY(cal_file) REFERENCES FILE(cal_id) ON DELETE CASCADE)";

	createTable(eventTable);

	var alarmTable = "CREATE TABLE ALARM ("
		+ "alarm_id INT AUTO_INCREMENT PRIMARY KEY,"
		+ "action VARCHAR(256) NOT NULL,"
		+ "`trigger` VARCHAR(256) NOT NULL,"
		+ "event INT NOT NULL,"
		+ "FOREIGN KEY(event) REFERENCES EVENT(event_id) ON DELETE CASCADE)";

	createTable(alarmTable);
}

// creates FILE, EVENT and ALARM tables, if they exist good, else print error if something else went wrong.
function createTable(sql) {
	connection.query(sql, function (err, result) {
		// if error code does not equal TABLE_EXISTS_ERROR then table is created, and or it already exists
		if (err) {
			if (err.code !== "ER_TABLE_EXISTS_ERROR") {
				console.log("Something went wrong: " + err);
			}
		}
	});
}

//store all files to database endpoint request
app.get('/storeAllFiles', function (req, res) {
	fs.readdir(__dirname + '/uploads', function (error, filenames) {
		if (error == null) {

			let data = [];
			var i = 0;
			for (i in filenames) {
				// create JSON for all files and save it into an array of calendar JSON
				let dbPtrPtr = ref.alloc(calPtrPtr);
				let status = sharedLib.createCalendar("./uploads/" + filenames[i], dbPtrPtr);
				let fileCalPtr = dbPtrPtr.deref();
				let fileJSON = sharedLib.calendarToJSON(fileCalPtr);
				let newJSON = {
					"status": status,
					"filename": filenames[i],
					"calToJSON": JSON.parse(fileJSON)
				};
				// only valid files are allowed to be added to the database
				if (newJSON.status === 0) {
					data.push(newJSON)
				}
			}

			// add files that do not exist in the database
			var i = 0;
			for (i in data) {
				checkIfFileExists(data[i]);
			}

			// timer set to let queries execute and then display database status.
			setTimeout(function(e) {
				res.send(getErrorMessage(1, "Stored files into database."));
			}, 2000);

		}
		else {
			console.log("Something went wrong: " + err);
		}
	});
});

// if file does not exist add file to database
function checkIfFileExists(file) {
	connection.query("SELECT * FROM FILE WHERE file_Name = '" + file.filename + "'", function (err, result) {
		// if error code does not equal TABLE_EXISTS_ERROR then table is created, and or it already exists
		if (err) {
			console.log("Something went wrong. " + err);
		}
		else if (result.length === 0) { // file does not exist so insert file data
			insertFile(file);
		}
	}); // end of checking if file name exists
}

// insert new file into FILE table
function insertFile(file) {
	connection.query("INSERT INTO FILE(file_Name, version, prod_id) VALUES ('" + file.filename + "'," + file.calToJSON.version + ",'" + file.calToJSON.prodID + "')", function (err, result) {
		if (err) {
			console.log("Something went wrong. " + err);
		}
		else {
			var cal_id = result.insertId;
			var i = 0;
			var events = file.calToJSON.events;
			for (i in events) {
				insertEvent(cal_id, events[i]);
			}
		}
	}); // end of inserting new file into FILE TABLE
}

// inserts an event into the EVENT table
function insertEvent(cal_id, event) {
	// get descriptions of properties and format startDateTime for database
	var location = getPropertyDescriptionFromList("location", event.properties);
	var organizer = getPropertyDescriptionFromList("organizer", event.properties);
	var start_time = formatStartTimeForDB(event.startDateTime);
	//query
	connection.query("INSERT INTO EVENT(summary, start_time, location, organizer, cal_file) VALUES"
		+ "('" + event.summary + "','" + start_time + "','" + location + "','" + organizer + "'," + cal_id + ")", function (err, result) {
			if (err) {
				console.log("Something went wrong. " + err);
			}
			else {
				var event_id = result.insertId;
				var i = 0;
				var alarms = event.alarms;
				for (i in alarms) {
					insertAlarm(event_id, alarms[i]);
				}
			}
		}); // end of inserting new event into EVENT TABLE
}

// inserts an alarm into ALARM table
function insertAlarm(event_id, alarm) {
	connection.query("INSERT INTO ALARM(action, `trigger`, event) VALUES"
		+ "('" + alarm.action + "','" + alarm.trigger + "'," + event_id + ")", function (err, result) {
			if (err) {
				console.log("Something went wrong. " + err);
			}
		}); // end of inserting new alarm into ALARM TABLE
}

// format startDateTime struct into a format for MYSQL database
function formatStartTimeForDB(DT) {
	var newFormat = DT.date.substring(0, 4) + "-" + DT.date.substring(4, 6) + "-" + DT.date.substring(6, 8) + "T" +
		DT.time.substring(0, 2) + ":" + DT.time.substring(2, 4) + ":" + DT.time.substring(4, 6);
	return newFormat;
}

// get property description from using property name
function getPropertyDescriptionFromList(property, list) {
	var i = 0;
	if (list === undefined) return 'NULL';
	for (i in list) {
		if (list[i].name.toUpperCase() === property.toUpperCase()) {
			return list[i].description;
		}
	}
	return 'NULL';
}

// clears all data in database
app.get('/clearAllData', function (req, res) {
	connection.query("DELETE FROM FILE", function (err, result) {
		if (err) {
			console.log("Something went wrong. " + err);
			res.send(getErrorMessage(-1, "Something went wrong when querying the database."));
		}
		else {
			res.send(getErrorMessage(1, "Cleared all data in the database."));
		}
	}); // end of delete all table data query
});

// gets database status
app.get('/getDBStatus', function (req, res) {
	var numFiles = 0;
	var numEvents = 0;
	var numAlarms = 0;
	connection.query("SELECT COUNT(*) AS cal_id FROM FILE", function (err, result) {
		if (err) {
			console.log("Something went wrong. " + err);
			res.send(getErrorMessage(-1, "Something went wrong when querying the database."));
		}
		else {
			numFiles = result[0].cal_id;
			connection.query("SELECT COUNT(*) AS event_id FROM EVENT", function (err, result) {
				if (err) {
					console.log("Something went wrong. " + err);
					res.send(getErrorMessage(-1, "Something went wrong when querying the database."));
				}
				else {
					numEvents = result[0].event_id;
					connection.query("SELECT COUNT(*) AS event FROM ALARM", function (err, result) {
						if (err) {
							console.log("Something went wrong. " + err);
							res.send(getErrorMessage(-1, "Something went wrong when querying the database."));
						}
						else {
							numAlarms = result[0].event;
							var data = {
								status: 1,
								message: "Database has " + pluralizer(numFiles, "file") + ", " + pluralizer(numEvents, "event") + ", and " + pluralizer(numAlarms, "alarm") + ".",
								numFiles: numFiles
							}
							res.send(data);
						}
					}); // end of count query for number of alarms in ALARM table
				}
			}); // end of count query for number of events in EVENT table
		}
	}); // end of count query for number of files in FILE table
});

// pluralizes a word, depending on the count
function pluralizer(count, toPluralize) {
	if (count === 1 || count === 0) {
		return (count + " " + toPluralize);
	}
	return (count + " " + toPluralize + "s");
}

// QUERIES
// 1. SELECT * FROM EVENT ORDER BY start_time;
// 2. SELECT * FROM FILE, EVENT WHERE ( file_Name = 'megaCal1.ics' AND FILE.cal_id = EVENT.cal_file ) ORDER BY EVENT.start_time; <-- replace the file_Name with the file you want to search
// 3. SELECT a.* FROM EVENT a JOIN (SELECT *, COUNT(start_time) FROM EVENT GROUP BY start_time HAVING COUNT(start_time) > 1) b ON a.start_time = b.start_time ORDER BY start_time;

// gets Query 1
app.get('/getQuery1', function (req, res) {
	connection.query("SELECT * FROM EVENT ORDER BY start_time", function (err, result) {
		if (err) {
			console.log("Something went wrong. " + err);
			res.send(getErrorMessage(-1, "Something went wrong when querying the database."));
		}
		else {
			var data = {
				status: 1,
				message: "Displaying all events sorted by start date.",
				result: result
			};
			console.log(data);
			// res.send(data);
		}
	});
});


app.listen(portNum);
console.log('Running app at localhost: ' + portNum);