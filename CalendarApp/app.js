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
		host		: 'dursley.socs.uoguelph.ca',
		user		: data.username,
		password	: data.password,
		database	: data.database
	});

	connection.connect(function(err) {
		// If error reprompt user for credentials
		if(err) {
			res.send( getErrorMessage(-1, "Error Connecting! Please try again.") );
		}
		// else success and create tables if needed.
		else {
			res.send( getErrorMessage(1, "Connected to database: " + data.database) );
			tablesToCreate();
		}
	});
});

// logouts out user and disconnects from database
app.get('/disconnectDatabase', function (req, res) {
	connection.end();
	res.send( getErrorMessage(1, "Disconnected database and logged out.") );
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
	connection.query(sql, function(err, result) {
		// if error code does not equal TABLE_EXISTS_ERROR then table is created, and or it already exists
		if(err) {
			if(err.code !== "ER_TABLE_EXISTS_ERROR"){
				console.log("Something went wrong: " + err);
			}
		}
	});
}

//store all files to database endpoint request
app.get('/storeAllFiles', function (req, res) {
	var data = req.query.validFiles;

	var i = 0;
	for(i in data){
		checkIfFileExists(data[i]);
	}
});

function checkIfFileExists(file){
	console.log(file);
	var sql = "SELECT * FROM FILE WHERE file_Name = '" + file.filename + "'";
	connection.query(sql, function(err, result) {
		// if error code does not equal TABLE_EXISTS_ERROR then table is created, and or it already exists
		if(err) {
			console.log("Something went wrong. "+err);
		}
		else if( result.length === 0 ) { // file does not exist so insert file data
			 
		}
	}); // Search if file exists return;
}


app.listen(portNum);
console.log('Running app at localhost: ' + portNum);