/*
	CalendarParser.c
	C file.
	By: Hrutvikkumar Patel (hrutvikk@uoguelph.ca)
	Student ID: 1002517
	Date: February 27, 2019
	CIS2750
	Assignment #2
*/

#include "CalendarParser.h"
#include "HelperFunctions.h"

/* Function to create a Calendar object based on the contents of an iCalendar file */
ICalErrorCode createCalendar(char* fileName, Calendar** obj) {
	ICalErrorCode status = OK;
	FILE* fp = NULL;

	/* checks extension(.ics), if it isnt empty or null and if file exists */
	if ((status = checkFile(&fp, fileName)) == INV_FILE) {
		*obj = NULL;
		return INV_FILE;
	}

	char* line = NULL;
	int currentScope = -1;
	bool atLeastOnce = false;

	/* initialize scope stack */
	Stack * scope = malloc(sizeof(Stack) + 200 * sizeof(int));
	scope->top = 0;
	scope->array[scope->top] = -1;

	/*Initialize Calendar Object.*/
	if ((status = initCalendarObj(obj)) != OK) {
		*obj = NULL;
		free(scope);
		fclose(fp);
		return status;
	}
		
	while( ( line = myFgets(fp, &status) ) != NULL ) {
		atLeastOnce = true;
		if (line[0] != ';') { /*disregards comments*/
			/**
			 * If currentScope is VCALENDAR then build Calendar, 
			 * else the file does not start with a BEGIN:VCALENDAR
			 */
			status = setScope(line, &scope, &currentScope);
			if (status == OK) {
				status = (currentScope == VCALENDAR) ? buildCalendar(fp, obj, &scope) : INV_CAL;
			}
			else {
				status = INV_CAL;
			}
		}
		/** 
		 * if an error occured from inside the previous if block, 
		 * free Calendar and break out of the loop
		 */
		if (status != OK) {
			deleteCalendar(*obj);
			*obj = NULL;
			freePtr(&line);
			break;
		}
		freePtr(&line);
	}
	/**
	 * if the while loop doesn't run at least once, 
	 * meaning the file is not formated as CRLF
	 */
	if (atLeastOnce == false ) {
		deleteCalendar(*obj);
		*obj = NULL;
		status = INV_FILE;
	}
	
	freePtr(&line);
	fclose(fp);
	free(scope);
	return status;
}

/* Parses and creates a calendar. */
ICalErrorCode buildCalendar( FILE* fp, Calendar** obj, Stack** scope) {
	char* pos;
	char* line = NULL;
	int currentScope = 0;
	ICalErrorCode status = OK;

	/*if the loop ends because of EOF then it will return true indicating EOF, else false, indicating INV_FILE*/
	while( ( line = myFgets(fp, &status) ) != NULL ) {

		status = setScope(line, scope, &currentScope);
		pos = parseLine(line, &status);

		/** 
		 * if an OTHER_ERROR occurs from setScope or parseLine, catch it
		 * and set it as the right required property error.
		 */
		if (status == OTHER_ERROR && strcmp( line, "" ) != 0) {
			if (pos == NULL) {
				if (strnCaseCmp("VERSION", line, 7)) status = INV_VER;
				else if (strnCaseCmp("PRODID", line, 6)) status = INV_PRODID;
			}
			else {
				int bytes = pos - line;
				if (strnCaseCmp("VERSION", line, bytes)) status = INV_VER;
				else if (strnCaseCmp("PRODID", line, bytes)) status = INV_PRODID; 
			}	
		}

		if (status == OK) {
			/*if currentSocpe == VCALENDAR then parse it accordingly.*/
			if (currentScope == VCALENDAR) {
				int bytes = pos - line;
					/*Parse VERSION property*/
					if (strnCaseCmp("VERSION", line, bytes)) {
						pos++;
						/*if VERSION does not equal 0.0, then VERSION is already initialized, thus DUP_VER*/
						if ((*obj)->version != 0.0) {
							status = DUP_VER;
						}
						else { /*else parse VERSION, if atof returns 0.0, then VERSION value is malformed.*/
							(*obj)->version = atof(pos); 
							/*If atof returns 0.0, then print invalid Version*/
							if ((*obj)->version == 0.0)
								status = INV_VER;
						}
					}
					/*Parse PRODID property*/
					else if (strnCaseCmp("PRODID", line, bytes)) {
						pos++;
						/*if prodID starts with null termiated value, then it has not been initialized yet*/
						if ((*obj)->prodID[0] == '\0') {
							/*if prodID value is NULL or empty, then INV_PRODID, else good.*/
							if (strcmp(pos, "") == 0 || *pos == '\0') status = INV_PRODID;
							strcpy( (*obj)->prodID, pos );
						}
						else status = DUP_PRODID; /*occurs if prodID is being initialized again*/
					}
					 /*if it isn't a PRODID or VERSION then it is a property*/
					else insertBack((*obj)->properties, buildProperty(line, pos));
			}
			else if ( currentScope == VEVENT ) status = buildEvent(fp, obj, scope);
		}
		/*checks if error has occured and break out of the loop*/
		if ( status != OK) {
			if ( status == OTHER_ERROR )
				status = INV_CAL;
			freePtr(&line);
			return status;
		}
		freePtr(&line);
	}
	/** if the scope does not return to -1, then it is an INV_CAL,
	 * means that the END:VCALENDAR tag is missing. 
	 */
	if ( currentScope != -1 ) return (INV_CAL);

	/* finally validate Calendar and return INV_CAL if it is not valid. */
	return validateCalRequirements(*obj);
}

/* Function to delete all calendar content and free all the memory. */
void deleteCalendar(Calendar* obj) {
	if (obj) {
		freeList(obj->events);
		freeList(obj->properties);
		free(obj);	
	}
}

/* Function to create a string representation of a Calendar object. */
char* printCalendar(const Calendar* obj) {

	/* if obj is NULL print Calendar is Null */
	if (obj == NULL) {
		char* str = malloc(sizeof(char) * 100);
		strcpy(str, "Calendar is NULL!\n");
		return str;
	}

	Property* property;
	Event* event;
	char* buf;
	char* temp;
	int len = 1000;
	char* tempBuf = malloc(sizeof(char) * 1000 );

	/* add required Calendar properties */
	buf = malloc( sizeof(char) * len);
	strcpy(buf, "BEGIN:VCALENDAR\n");
	sprintf(tempBuf, "VERSION:%.1f\n",obj->version);
	strcat(buf, tempBuf);
	sprintf(tempBuf, "PRODID:%s\n",obj->prodID);
	strcat(buf, tempBuf); 

	/*Add Calendar Optional Properties*/
	ListIterator properties = createIterator(obj->properties);
	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		temp = printProperty(property);
		buf = realloc(buf, sizeof(char) * ( len += strlen(temp) + 2 ) );
		strcat(buf, temp);
		strcat(buf, "\n");
		free(temp);
	}

	/*Add Events*/
	ListIterator events = createIterator(obj->events);
	while ( (event = (Event*) nextElement(&events)) != NULL ) {
		temp = printEvent(event);
		buf = realloc(buf, sizeof(char) * ( len += strlen(temp) + 100 ) );
		strcat(buf, temp);
		free(temp);
	}

	buf = realloc(buf, sizeof(char) * (len += 100));
	strcat(buf, "END:VCALENDAR\n");
	free(tempBuf);
	return buf;
}

/* Function to "convert" the ICalErrorCode into a humanly redabale string. */
char* printError(ICalErrorCode err) {
	const char errorCodes[12][200] = 
	{
		"OK", /*OK*/
		"Invalid file", /*INV_FILE*/
		"Invalid calendar", /*INV_CAL*/
		"Invalid version", /*INV_VER*/
		"Duplicate version", /*DUP_VER*/
		"Invalid product ID", /*INV_PRODID*/
		"Duplicate product ID", /*DUP_PRODID*/
		"Invalid event", /*INV_EVENT*/
		"Invalid date time", /*INV_DT*/
		"Invalid alarm", /*INV_ALARM*/
		"Write error", /*WRITE_ERROR*/
		"Something went wrong" /*OTHER_ERROR*/
	};
	char* errorToPrint;
	errorToPrint = malloc(sizeof(char) * (strlen(errorCodes[err]) + 1) );
	strcpy(errorToPrint, errorCodes[err]);
	errorToPrint[ strlen(errorToPrint) ] = '\0';
	return errorToPrint;
}

/* Function to writing a Calendar object into a file in iCalendar format. */
ICalErrorCode writeCalendar(char* fileName, const Calendar* obj) {
	ICalErrorCode status = OK;
	FILE* fp;

	if (obj == NULL) return WRITE_ERROR;
	if (fileName == NULL) return WRITE_ERROR;
	if ( (status = checkFileExtension(fileName)) == INV_FILE ) return WRITE_ERROR;
	if ((fp = fopen(fileName, "w")) == NULL ) return WRITE_ERROR;

	Property* property;
	Event* event;
	char* buf;
	char* tempBuf;
	int len = 1000;

	if ((tempBuf = malloc(sizeof(char)*len)) == NULL ) {
		fclose(fp);
		return WRITE_ERROR;
	}

	/* add required Calendar properties */
	if ((buf = malloc(sizeof(char)*len)) == NULL ) {
		freePtr(&tempBuf);
		fclose(fp);
		return WRITE_ERROR;
	}
	  
	strcpy(buf, "BEGIN:VCALENDAR\r\n");
	sprintf(tempBuf, "VERSION:%.1f\r\n",obj->version);
	strcat(buf, tempBuf);
	sprintf(tempBuf, "PRODID:%s\r\n",obj->prodID);
	strcat(buf, tempBuf); 

	if (fprintf(fp,"%s", buf) < 0) {
		freeWriteState(&fp, &buf, &tempBuf);
		return WRITE_ERROR;
	}
	freePtr(&buf);

	/*Add Calendar Optional Properties*/
	ListIterator properties = createIterator(obj->properties);
	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		if ( (status = writeProperty(fp, property)) == WRITE_ERROR ) {
			freeWriteState(&fp, &buf, &tempBuf);
			return WRITE_ERROR;
		}
	}

	/*Add Events*/
	ListIterator events = createIterator(obj->events);
	while ( (event = (Event*) nextElement(&events)) != NULL ) {
		if ( (status = writeEvent(fp, event)) == WRITE_ERROR ) {
			freeWriteState(&fp, &buf, &tempBuf);
			return WRITE_ERROR;
		}
	}

	if ( (buf = malloc(sizeof(char)*len)) == NULL ) {
		freeWriteState(&fp, &buf, &tempBuf);
		return WRITE_ERROR;
	}
	strcpy(buf, "END:VCALENDAR\r\n");

	if ( fprintf(fp,"%s", buf) < 0 ) status = WRITE_ERROR;
	freeWriteState(&fp, &buf, &tempBuf);
	return status;
}

/* Writes an event component to a file. */
ICalErrorCode writeEvent(FILE* fp, Event* event) {
	ICalErrorCode status = OK;

	char* buf = NULL;
	Property* property = NULL;
	Alarm* alarm = NULL;
	char* tempBuf;
	int len = 1000;
	char* date = NULL;

	if ((tempBuf = malloc(sizeof(char)*len)) == NULL) return WRITE_ERROR;

	if ((buf = malloc(sizeof(char)*len)) == NULL) {
		freePtr(&tempBuf);
		return WRITE_ERROR;
	}

	strcpy(buf, "BEGIN:VEVENT\r\n");
	sprintf(tempBuf, "UID:%s\r\n", event->UID);
	strcat(buf, tempBuf);

	date = printDate(&(event->creationDateTime));
	sprintf(tempBuf, "DTSTAMP:%s\r\n", date );
	strcat(buf, tempBuf);
	freePtr(&date);

	date = printDate(&(event->startDateTime));
	sprintf(tempBuf, "DTSTART:%s\r\n", date );
	strcat(buf, tempBuf);
	freePtr(&date);
	freePtr(&tempBuf);

	if ( fprintf(fp,"%s", buf) < 0 )
		return WRITE_ERROR;
	freePtr(&buf);

	/* Print Calendar Properties*/
	ListIterator properties = createIterator(event->properties);
	while ( ( property = (Property*) nextElement(&properties) ) != NULL ) {
		if ( (status = writeProperty(fp, property)) == WRITE_ERROR ) {
			freeWriteState(&fp, &buf, &tempBuf);
			return WRITE_ERROR;
		}
	}

	/* Print alarm properties */
	ListIterator alarms = createIterator(event->alarms);
	while ( ( alarm = (Alarm*) nextElement(&alarms) ) != NULL ) {
		if ( (status = writeAlarm(fp, alarm)) == WRITE_ERROR ) {
			freeWriteState(&fp, &buf, &tempBuf);
			return WRITE_ERROR;
		}
	}

	if ( (buf = malloc(sizeof(char)*len)) == NULL ) {
		freePtr(&buf);
		freePtr(&tempBuf);
		return WRITE_ERROR;
	}

	strcpy(buf, "END:VEVENT\r\n");

	if ( fprintf(fp,"%s", buf) < 0 )
		status =  WRITE_ERROR;

	freePtr(&buf);
	freePtr(&tempBuf);
	freePtr(&date);

	return status;
}

/* Writes an alarm component to a file. */
ICalErrorCode writeAlarm(FILE* fp, Alarm* alarm) {
	ICalErrorCode status = OK;

	Property* property = NULL;
	int len = 1000;
	char* buf = NULL;
	char* tempBuf = NULL;

	if ( (tempBuf = malloc(sizeof(char)*len)) == NULL ) return WRITE_ERROR;

	if ( (buf = malloc(sizeof(char)*len)) == NULL ) {
		freePtr(&tempBuf);
		return WRITE_ERROR;
	}

	/* Print alarm required properties. */
	strcpy(buf, "BEGIN:VALARM\r\n");
	sprintf(tempBuf, "ACTION:%s\r\n", alarm->action );
	strcat(buf, tempBuf);
	sprintf(tempBuf, "TRIGGER;%s\r\n", alarm->trigger);
	strcat(buf, tempBuf);
	freePtr(&tempBuf);

	if ( fprintf(fp,"%s", buf) < 0 ) return WRITE_ERROR;
	freePtr(&buf);

	/*Print alarm properties.*/
	ListIterator properties = createIterator(alarm->properties);
	while ( ( property = (Property*) nextElement(&properties) ) != NULL ) {
		if ( (status = writeProperty(fp, property)) == WRITE_ERROR ) {
			freeWriteState(&fp, &buf, &tempBuf);
			return WRITE_ERROR;
		}
	}

	if ( (buf = malloc(sizeof(char)*len)) == NULL ) {
		freePtr(&buf);
		freePtr(&tempBuf);
		return WRITE_ERROR;
	}

	strcpy(buf, "END:VALARM\r\n");

	if ( fprintf(fp,"%s", buf) < 0 ) status =  WRITE_ERROR;

	freePtr(&buf);
	freePtr(&tempBuf);
	return status;
}

/* Writes a property to a file. */
ICalErrorCode writeProperty(FILE* fp, Property* property) {
	ICalErrorCode status = OK;
	char* buf = NULL;
	char* pos = NULL;
	int len = 0;
	char seperator[2];

	if ( (buf = realloc(buf, (sizeof(char)*( len += strlen(property->propName) + 2))) ) == NULL ) {
		freePtr(&buf);
		return WRITE_ERROR;
	}

	strcpy(buf, property->propName );
	pos = colonOrSemi(property->propDescr);
	strcpy(seperator, (pos == NULL ? ":\0" : ";\0")); 	/* Print colon or Semi colon. */
	strcat(buf, seperator);

	if ( (buf = realloc(buf, (sizeof(char) * ( len += strlen(property->propDescr) + 10)) )) == NULL ) {
		freePtr(&buf);
		return WRITE_ERROR;
	}
	strcat(buf, property->propDescr);
	strcat(buf, "\r\n");

	if ( fprintf(fp,"%s", buf) < 0 ) status =  WRITE_ERROR;
	freePtr(&buf);
	return status;
}

void freeWriteState(FILE** fp, char** buf, char** tempBuf) {
	fclose(*fp);
	*fp = NULL;
	freePtr(buf);
	freePtr(tempBuf);
}

/* Function to validate an existing Calendar object. */
ICalErrorCode validateCalendar(const Calendar* obj) {
	ICalErrorCode status = OK;
	bool calscale = false;
	bool method = false;

	if( (status = validateCalRequirements(obj)) != OK ) return status; /* Required Calendar properties and compnents*/

	Property* property;
	ListIterator properties = createIterator(obj->properties); /* Iterate for Optional Properties */

	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		if( isStaticArrayInvalid(property->propName, 200) || isEmpty(property->propDescr) ) return INV_CAL;
		if(!isCalProp(property->propName)) return INV_CAL;

		if (strCaseCmp("CALSCALE", property->propName )) {
			if (calscale == false) calscale = true;
			else status = INV_CAL;
		}
		if (strCaseCmp("METHOD", property->propName)) {
			if (method == false) method = true;
			else status = INV_CAL;
		}
		if(status == INV_CAL) return status;
	}

	Event* event;
	ListIterator events = createIterator(obj->events); /* Iterate for Optional Properties */

	while ( (event = (Event*) nextElement(&events)) != NULL ) {
		if( (status = validateEvent(event)) != OK ) return status;
	}

	return status;
}

/* Validates Calendar Required Properties. */
ICalErrorCode validateCalRequirements(const Calendar* obj) {
	if(obj == NULL) return INV_CAL;
	if(obj->events == NULL || obj->events->length == 0) return INV_CAL;
	if(obj->properties == NULL) return INV_CAL;
	if (obj->version <= 0.0) return INV_CAL;
	if (isStaticArrayInvalid(obj->prodID, 1000)) return INV_CAL;
	return OK;
}

/* Function to validate an existing Event object. */
ICalErrorCode validateEvent(const Event* event) {
	ICalErrorCode status = OK;
	Property* property;
	int i = 0;
	int size = 14;
	bool duration = false;
	bool dtend = false; 

	const char* optionalOnlyOnceProps[] =
	{
        "CLASS", "CREATED", "DESCRIPTION", "GEO", "LAST-MODIFIED", "LOCATION", "ORGANIZER", "PRIORITY",
        "SEQUENCE", "STATUS", "SUMMARY", "TRANSP", "URL", "RECURRENCE-ID"
	};

	int optionalOnlyOnceCounter[size];
	for(i=0; i < size; i++) 
		optionalOnlyOnceCounter[i] = 0;

	if( (status = validateEventRequirements(event)) != OK ) return status;

	if( strlen(event->creationDateTime.date) != 8 ) return INV_EVENT;
	if( strlen(event->creationDateTime.time) != 6 ) return INV_EVENT;
	if( strlen(event->startDateTime.date) != 8 ) return INV_EVENT;
	if( strlen(event->startDateTime.time) != 6 ) return INV_EVENT;

	ListIterator properties = createIterator(event->properties); /* Iterate for Optional Properties */

	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		if( isStaticArrayInvalid(property->propName, 200) || isEmpty(property->propDescr) ) return INV_EVENT;
		if(!isEventProp(property->propName)) return INV_EVENT;

		if( !OccursOnlyOncePropCounter(property->propName, optionalOnlyOnceProps, optionalOnlyOnceCounter, size) )
			return INV_EVENT;

		/* Assuming that duration and dtend are optional and either duration or dtend can occur. Assuming multiple times is fine. */
		if(strCaseCmp("DURATION", property->propName)) duration = true;
		if(strCaseCmp("DTEND", property->propName)) dtend = true;
		if(duration && dtend) return INV_EVENT;

		if(status == INV_EVENT) return status;
	}

	Alarm* alarm;
	ListIterator alarms = createIterator(event->alarms); /* Iterate for Optional Properties */

	while ( (alarm = (Alarm*) nextElement(&alarms)) != NULL ) {
		if( (status = validateAlarm(alarm)) != OK ) return status;
	}

	return status;
}

/* Validates required event properties */
ICalErrorCode validateEventRequirements(const Event* event) {
	if (isStaticArrayInvalid(event->UID, 1000)) return INV_EVENT;
	if (isEmpty(event->creationDateTime.date)) return INV_EVENT;
	if (isEmpty(event->startDateTime.date)) return INV_EVENT;
	if (event->properties == NULL) return INV_EVENT;
	if (event->alarms == NULL) return INV_EVENT;
	return OK;
}

/* Function to validate an existing Alarm object. */
ICalErrorCode validateAlarm(const Alarm* alarm) {
	ICalErrorCode status = OK;
	Property* property;
	bool duration = false;
	bool repeat = false;
	bool attach = false;

	if( (status = validateAlarmRequirements(alarm)) != OK ) return status;

	if(!strCaseCmp("AUDIO", alarm->action)) return INV_ALARM;

	ListIterator properties = createIterator(alarm->properties); /* Iterate for Optional Properties */

	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		if( isStaticArrayInvalid(property->propName, 200) || isEmpty(property->propDescr) ) return INV_ALARM;
		if(!isAlarmProp(property->propName)) return INV_ALARM;

		if (strCaseCmp("DURATION", property->propName )) {
			if (duration == false) duration = true;
			else status = INV_ALARM;
		}
		if (strCaseCmp("REPEAT", property->propName)) {
			if (repeat == false) repeat = true;
			else status = INV_ALARM;
		}
		if (strCaseCmp("ATTACH", property->propName)) {
			if (attach == false) attach = true;
			else status = INV_ALARM;
		}

		if(status == INV_ALARM) return status;
	}

	if((!repeat && duration) || (repeat && !duration)) return INV_ALARM;

	return status;
}

/* Validate Alarm requirements. */
ICalErrorCode validateAlarmRequirements(const Alarm* alarm) {
	if (isStaticArrayInvalid(alarm->action, 200)) return INV_ALARM;
	if (isEmpty(alarm->trigger)) return INV_ALARM;
	if (alarm->properties == NULL) return INV_ALARM;
	return OK;
}

/* Checks if property is a vlid calendar property that can occur in property list. */
bool isCalProp(const char* s) {
	if(strCaseCmp("CALSCALE", s)) return true;
	if(strCaseCmp("METHOD", s)) return true;
	return false;
}

/* Checks if a property is a event property */
bool isEventProp(const char* s) {
	const char properties[28][20] = 
	{
        "CLASS",
        "CREATED",
        "DESCRIPTION",
        "GEO",
        "LAST-MODIFIED",
        "LOCATION",
        "ORGANIZER",
        "PRIORITY",
        "SEQUENCE",
        "STATUS",
        "SUMMARY",
        "TRANSP",
        "URL",
        "RECURRENCE-ID",
        "RRULE",
        "DTEND",
        "DURATION",
        "ATTACH",
        "ATTENDEE",
        "CATEGORIES",
        "COMMENT",
        "CONTACT",
        "EXDATE",
        "RESOURCES",
        "RDATE",
		"RELATED-TO"
	};
	int i = 0;
	for(i = 0; i < 28; i++)
		if(strCaseCmp(properties[i], s)) return true;
	return false;
}

/* Checks if a property is a alarm property */
bool isAlarmProp(const char* s) {
	if(strCaseCmp("DURATION", s)) return true;
	if(strCaseCmp("REPEAT", s)) return true;
	if(strCaseCmp("ATTACH", s)) return true;
	return false;
}

/* calculates if a property only occurs once. Takes a list of unique properties and
 a corresponding int array that holds the number of occurences for each unique property */
bool OccursOnlyOncePropCounter(const char* propName, const char** properties, int* count, int size) {
	int i = 0;

	for(i = 0; i < size; i++) {
		if(strCaseCmp(propName, properties[i])) {
			if(count[i] == 0) {
				count[i] = 1;
				return true;
			}
			else return false;
		}
	}
	return true;
}

/* Initializes a Calendar Object. */
ICalErrorCode initCalendarObj(Calendar** obj) {
	if ( ( (*obj) = malloc(sizeof(Calendar)) ) == NULL ) return OTHER_ERROR;
	(*obj)->events = initializeList(printEvent, deleteEvent, compareEvents);
	(*obj)->prodID[0] = '\0';
	(*obj)->version = 0.0;
	(*obj)->properties = initializeList(printProperty, deleteProperty, compareProperties);
	return OK;
}

/*-------EVENTS HELPER FUNCTION-------*/

/* Creates a new event with properties. */
ICalErrorCode buildEvent( FILE* fp, Calendar** obj, Stack** scope) {
	char* pos;
	char* line = NULL;
	int currentScope = 1;
	ICalErrorCode status = OK;
	Event * newEvent = initEvent();

	/*if the loop ends because of EOF then it will return true indicating EOF, else false, indicating INV_FILE*/
	while( ( line = myFgets(fp, &status) ) != NULL ) {

		status = setScope(line, scope, &currentScope);
		pos = parseLine(line, &status);

		/** 
		 * if an OTHER_ERROR occurs from setScope or parseLine, catch it
		 * and set it as the right required property error.
		 */
		if ( status == OTHER_ERROR && strcmp( line, "" ) != 0 && pos != NULL ) {
			if ( strcmp( pos, "" ) == 0 ) {
				status = INV_EVENT;
			}
			else {
				int bytes = pos - line;	
				if ( strnCaseCmp("UID", line, bytes) ) 			status = INV_EVENT;
				else if ( strnCaseCmp("DTSTAMP", line, bytes) ) status = INV_DT; 
				else if ( strnCaseCmp("DTSTART", line, bytes) ) status = INV_DT; 
			}		
		}

		if ( status == OK ) {
			/* start parsing for an event */
			if ( currentScope == VEVENT ) {
				int bytes = pos - line; /* parse required event properties, if date time error occurs return INV_DT */
				if ( strnCaseCmp("UID", line, bytes) ) {
					pos++;
					strcpy(newEvent->UID, pos);
				}
				else if ( strnCaseCmp("DTSTAMP", line, bytes) ) {
					status = addDateTime( &(newEvent->creationDateTime), line );
				}
				else if ( strnCaseCmp("DTSTART", line, bytes) ) {
					status = addDateTime( &(newEvent->startDateTime), line );
				}
				else {
					insertBack(newEvent->properties, buildProperty(line, pos));
				}
			} 
			else if ( currentScope == VALARM ) { 			/* start parsing for an alarm */
				status = buildAlarm(fp, newEvent, scope);			
			}
			else { 	/* else if the scope isn't either event or alarm, it is an error break. */
				freePtr(&line);
				break;
			}	
		}
		else if (status != OK) {
			if ( status == OTHER_ERROR ) status = INV_EVENT;
			deleteEvent(newEvent);
			freePtr(&line);
			return status;
		}

		if (status != OK) {
			deleteEvent(newEvent);
			freePtr(&line);
			return status;
		}
		freePtr(&line);
	}

	/* Validate Event Requirements, it error return OK or INV_EVENT */
	status = validateEventRequirements(newEvent);
	if (status != OK) {
		deleteEvent(newEvent);
		return status;
	}
	/* If all is good, insert event */
	insertBack((*obj)->events, newEvent);
	return status;
}


/* Delete Event. */
void deleteEvent(void* toBeDeleted) {
	Event* event = (Event*) toBeDeleted;
	freeList(event->properties);
	freeList(event->alarms);
	free(event);
	event = NULL;
}

/* Compare events. */
int compareEvents(const void* first, const void* second) {
	return 0;
}

/* Print Event. */
char* printEvent(void* toBePrinted) {
	if (toBePrinted == NULL) return NULL;

	char* buf = NULL;
	Property* property = NULL;
	Alarm* alarm = NULL;
	char* tempBuf;
	int len = 1000;
	char* date = NULL;

	tempBuf = malloc(sizeof(char)*1000);
	Event* event = (Event*) toBePrinted;

	/* print required event properties. */

	buf = malloc( sizeof(char) * len);
	strcpy(buf, "BEGIN:VEVENT\n");
	sprintf(tempBuf, "UID:%s\n", event->UID);
	strcat(buf, tempBuf);

	date = printDate(&(event->creationDateTime));
	sprintf(tempBuf, "DTSTAMP:%s\n", date );
	strcat(buf, tempBuf);
	free(date);

	date = printDate(&(event->startDateTime));
	sprintf(tempBuf, "DTSTART:%s\n", date );
	strcat(buf, tempBuf);
	free(date);
	free(tempBuf);

	/* Print Calendar Properties*/
	ListIterator properties = createIterator(event->properties);
	while ( ( property = (Property*) nextElement(&properties) ) != NULL ) {
		char* temp = printProperty(property);
		buf = realloc(buf, sizeof(char) * ( len += strlen(temp) + 2 ) );
		strcat(buf, temp);
		strcat(buf, "\n");
		free(temp);
	}

	/* Print alarm properties */
	ListIterator alarms = createIterator(event->alarms);
	while ( ( alarm = (Alarm*) nextElement(&alarms) ) != NULL ) {
		char* temp = printAlarm(alarm);
		buf = realloc(buf, sizeof(char) * ( len += strlen(temp) + 2 ) );
		strcat(buf, temp);
		free(temp);
	}
	buf = realloc(buf, sizeof(char)*(len += 100 ));
	strcat(buf, "END:VEVENT\n");
	return buf;
}

/* Initialize an event. */
Event* initEvent() {
	Event *newEvent = malloc(sizeof(Event));
	newEvent->properties = initializeList(printProperty, deleteProperty, compareProperties);
	newEvent->alarms = initializeList(printAlarm, deleteAlarm, compareAlarms);
	newEvent->UID[0] = '\0';
	newEvent->creationDateTime.date[0] = '\0';
	newEvent->startDateTime.date[0] = '\0';
	return newEvent;
}

/*--------ALARM HELPER FUNCTION-------*/

/* Delete Alarm. */
void deleteAlarm(void* toBeDeleted) {	
	Alarm* alarm = (Alarm*) toBeDeleted;
	freeList(alarm->properties);
	freePtr(&(alarm->trigger));
	free(alarm);
	alarm = NULL;
}

/* Compare alarms. */
int compareAlarms(const void* first, const void* second) {
	return 0;
}

/* Print Alarm. */
char* printAlarm(void* toBePrinted) {
	char* buf = NULL;
	Property* property = NULL;
	int len = 1000;
	char* temp = NULL;
	char* tempBuf = NULL;

	if (toBePrinted == NULL) return NULL;

	Alarm* alarm = (Alarm*) toBePrinted;

	buf = malloc( sizeof(char) * len);
	tempBuf = malloc( sizeof(char) * len);

	/* Print alarm required properties. */
	strcpy(buf, "BEGIN:VALARM\n");
	sprintf(tempBuf, "ACTION:%s\n", alarm->action );
	strcat(buf, tempBuf);
	sprintf(tempBuf, "TRIGGER:%s\n", alarm->trigger);
	strcat(buf, tempBuf);
	free(tempBuf);

	/*Print alarm properties.*/
	ListIterator properties = createIterator(alarm->properties);
	while ( ( property = (Property*) nextElement(&properties) ) != NULL ) {
		temp = printProperty(property);
		buf = realloc(buf, sizeof(char) * ( len += strlen(temp) + 2 ) );
		strcat(buf, temp);
		strcat(buf, "\n");
		free(temp);
	}
	buf = realloc(buf, sizeof(char)*(len += 100));
	strcat(buf, "END:VALARM\n");

	return buf;
}

/* Initialize an alarm. */
Alarm* initAlarm() {
	Alarm* newAlarm = malloc( sizeof(Alarm) );
	newAlarm->properties = initializeList(printProperty, deleteProperty, compareProperties);
	newAlarm->trigger = NULL;
	newAlarm->action[0] = '\0';
	return newAlarm;
}

/* Create an Alarm.*/
ICalErrorCode buildAlarm( FILE* fp, Event* event, Stack** scope) {
	char* pos;
	char* line = NULL;
	int currentScope = 2;
	ICalErrorCode status = OK;
	Alarm* newAlarm = initAlarm();

	/*if the loop ends because of EOF then it will return true indicating EOF, else false, indicating INV_FILE*/
	while( ( line = myFgets(fp, &status) ) != NULL ) {
		status = setScope(line, scope, &currentScope);
		pos = parseLine(line, &status);
		
		/* If all good parse content line for an alarm. */
		if (status == OK) {
			if (currentScope == VALARM) {
				int bytes = pos - line;

				if (strnCaseCmp("ACTION", line, bytes)) {
					pos++;
					strcpy(newAlarm->action, pos);
				}
				else if (strnCaseCmp("TRIGGER", line, bytes)) {
					pos++;
					newAlarm->trigger = malloc(sizeof(char)* (strlen(pos)+1));
					strcpy(newAlarm->trigger, pos);
				}
				else insertBack(newAlarm->properties, buildProperty(line, pos));
			}
			else {
				freePtr(&line);
				break;
			}
		}
		/* Free if anything goes wrong and return to print error. */
		else if (status != OK) {
			if (status == OTHER_ERROR) status = INV_ALARM;
			deleteAlarm(newAlarm);
			freePtr(&line);
			return status;			
		}
		freePtr(&line);
	}

	status = validateAlarmRequirements(newAlarm);
	if (status != OK) {
		deleteAlarm(newAlarm);
		return status;
	}
	insertBack(event->alarms, newAlarm);
	return status;
}


/*--------PROPERTY HELPER FUNCTIONS---------*/

/* Delete property. */
void deleteProperty(void* toBeDeleted) {
	if (toBeDeleted == NULL) return;
	Property* property = (Property*) toBeDeleted;
	free(property);
	property = NULL;
}

/* Compare Properties. */
int compareProperties(const void* first, const void* second) {
	return 0;
}

/* Print Property. */
char* printProperty(void* toBePrinted) {
	char* str = NULL;
	char* pos = NULL;
	int len = 0;

	if (toBePrinted == NULL) return NULL;

	Property* property = (Property*) toBePrinted;

	if ( (str = realloc(str, (sizeof(char)*( len += strlen(property->propName) + 2))) ) == NULL ) {
		free(str);
		return NULL;
	}

	/* Print colon or Semi colon. */
	strcpy(str, property->propName);
	pos = colonOrSemi(property->propDescr);

	if ( pos != NULL ) strcat(str, ";");
	else strcat(str, ":");

	if ( (str = realloc(str, (sizeof(char) * ( len += strlen(property->propDescr) + 2)) )) == NULL ) {
		free(str);
		return NULL;
	}

	strcat(str, property->propDescr);
	return str;
}

/* Creates a new property struct. */
Property* buildProperty(char* line, char* pos) {
	int bytes = pos - line;
	if ( strnCaseCmp("BEGIN", line, bytes) == false && strnCaseCmp("END", line, bytes) == false  ) {
		Property * newProp = malloc(sizeof(Property) + (strlen(pos)+1)*sizeof(char));
		strncpy(newProp->propName, line, bytes);
		newProp->propName[bytes] = '\0';
		strcpy(newProp->propDescr, ++pos);
		return newProp;
	}
	return NULL;
}

/*----------DATE HELPER FUNCTIONS--------*/

/* Adds a Date Time property. Parses date time content line to make sure 
it is valid (Only valid for form 1 & 2, not for 3 and others.) */
ICalErrorCode addDateTime(DateTime* date, char* line ) {
	ICalErrorCode status = OK;
	char* pos = parseLine(line, &status);
	char UTC;
	char* temp = line;

	 /* 3rd Date-time form is invalid for assignment purposes. */
	if (pos) {
		if ( *pos == ':' ) {
			pos++;
			temp = pos;
		}
		else return INV_DT;
	}

	int len = strlen(temp);

	/* If UTC is included len is 16, else 15, if anything other than that invalid */
	if ( len != 16 && len != 15 )
		return INV_DT;

	/** scanf for three date time properties:
	 *  date, time and UTC else scanf for only two, otherwise it is an INV_DT 
	 */
	if ( sscanf(temp, "%8sT%6s%c", (*date).date, (*date).time, &UTC ) == 3 ) {
		if (UTC == 'Z') (*date).UTC = true;
	}
	else if ( sscanf(temp, "%8sT%6s", (*date).date, (*date).time) == 2 ) {
		(*date).UTC = false;
	}
	else return (status = INV_DT);

	(*date).date[8] = '\0';
	(*date).time[6] = '\0';

	/* Check if all date and time numbers are digits, else error */
	if ( isDigit( (*date).date ) == 0 || isDigit( (*date).time ) == 0 ) 
		return INV_DT;

	return status;
}

/* Delete Date */
void deleteDate(void* toBeDeleted) {
}

/* Compare Dates. */
int compareDates(const void* first, const void* second) {
	return 0;
}

/* Print Date. */
char* printDate(void* toBePrinted) {
	char* str = malloc(sizeof(char)*1000);
	if (toBePrinted == NULL) return NULL;

	DateTime* dateTime = (DateTime*) toBePrinted;
	sprintf(str, "%sT%s", dateTime->date, dateTime->time);

	if (dateTime->UTC) strcat(str,"Z");
	return str;
}

/*----------My Own functions-----------*/

/* My own fgets, that instead returns unfolded content lines, by seeking CRLF flag.
 * It removes comments as it parses, if they occur anywhere, before, after or even between folded lines. */
char* myFgets(FILE* fp, ICalErrorCode* status) {
	char c;
	int i = 0;
	char* p = NULL;
	char* temp = NULL;
	bool endOfLine = false; /* flag for when pointer points to '\r' character */
	bool contentLine = false;
	int numLines = 1;
	bool firstRun = true;

	p = malloc(sizeof(char) * (80*numLines));
	temp = p;

	/* get max bytes or upto end of line, NOT TILL A NEWLINE */
	while(contentLine == false) {
		/* realloc memory if i reaches size of 80 characters x numLines */
		if ( i == (80*numLines) ) {
			numLines++;
			p = realloc(p, sizeof(char) * (80*numLines));
			temp = p;
		}

		/* get another character, if it is EOF and not end of a content line, return NULL */
		c = fgetc(fp);
		if ( c == EOF && endOfLine != true ) {
			freePtr(&p);
			return NULL;
		}
		p[i] = c;

		/* On the firstRun check if first char is a comment, if so remove it. */
		if ( firstRun == true && p[i] == ';' ) {
			*status = removeComments(fp);
			firstRun = false;
			i--;
		}
		else firstRun = false;
		
		/* detects endOfLine flag, so CRLF */
		if ( i >= 1 && p[i] == '\n' && p[i-1] == '\r' ) {
			endOfLine = true;
			i--;
			continue;
		}

		/* if endOFline is detected, check of folding, and unfold if so */
		if (endOfLine == true) {
			if ( p[i] == ' ' || p[i] == '\t' ) { /* if the next character is space or tab remove it and   */
				endOfLine = false;
				continue;
			}
			else if (p[i] == ';') { /* If after the CRLF it is a semi-colon remove the comment */
				*status = removeComments(fp);
				i--;
			}
			else {
				p[i] = '\0';
				fseek(fp,-1,SEEK_CUR);
				contentLine = true;
			}
		}
		i++;
	}
	/* Move pointer back to the beginning,
	then check if pointer size == dst size, or character == EOF */
	p = temp;
	return (p);
}

/* Remove Comments. */
ICalErrorCode removeComments(FILE* fp) {
	ICalErrorCode status = OK;
	char c;
	int i = 0;
	char* p = NULL;
	bool endOfLine = false; /* flag for when pointer points to '\r' character */
	int numLines = 1;
	p = malloc(sizeof(char) * (80*numLines) );

	while(1) {
		/* realloc memory if i reaches size of 80 characters x numLines */
		if  (i == (80*numLines) ) {
			numLines++;
			p = realloc(p, sizeof(char) * (80*numLines) );
		}
		/* get another character, if it is EOF and not end of a content line, return NULL */
		c = fgetc(fp);
		if ( c == EOF && endOfLine != true ) {
			freePtr(&p);
			return OK;
		}
		p[i] = c;
		
		/* detects endOfLine flag, so CRLF */
		if ( i >= 1 && p[i] == '\n' && p[i-1] == '\r' ) {
			endOfLine = true;
			i--;
			continue;
		}

		/* if endOFline is detected, check for folding, and unfold if so */
		if (endOfLine == true) {
			if (p[i] == ';') {
				endOfLine = false;
				continue;
			}
			else {
				freePtr(&p);
				fseek(fp,-1,SEEK_CUR);
				return OK;
			}
		}
		i++;
	}
	return status = OK;
}

/* Determines scope of the file, In terms of an EVENT, ALARM, CALENDER etc. */
ICalErrorCode setScope(char* line, Stack ** scope, int* currentScope) {
	ICalErrorCode status = OK;
	char * pos = parseLine(line, &status);

	if (status!=OK) return status;
	int bytes = pos - line;

	if ( strnCaseCmp("BEGIN", line, bytes) ) {
		/* increment position to not point to ':' or ';', and then compare description */
		pos++;
		status = 
			strCaseCmp("VCALENDAR", pos) ? push(scope, VCALENDAR) :
			strCaseCmp("VEVENT", pos) 	 ? push(scope, VEVENT)    :
			strCaseCmp("VALARM", pos)    ? push(scope, VALARM)    :
			scopeSpecificError(*currentScope);
	}
	else if ( strnCaseCmp("END", line, bytes) ) {
		/* increment position to not point to ':' or ';', and then compare description */
		pos++;
		status = 
			strCaseCmp("VCALENDAR", pos) ? pop(scope, VCALENDAR) :
			strCaseCmp("VEVENT", pos) 	 ? pop(scope, VEVENT)    :
			strCaseCmp("VALARM", pos)    ? pop(scope, VALARM)    :
			scopeSpecificError(*currentScope);
	}

	*currentScope = (*scope)->array[ (*scope)->top ];
	return status;
}

/* Returns an error that refers to the currentScope. */
ICalErrorCode scopeSpecificError(int currentScope) {
	return 
		currentScope == VCALENDAR ? INV_CAL    :
		currentScope == VEVENT 	  ? INV_EVENT  :
		currentScope == VALARM    ? INV_ALARM  :
		OTHER_ERROR;
}

/* Push function to determine valid scoping. */
ICalErrorCode push( Stack** scope, int newScope ) {
	int top = (*scope)->top;
	int currentScope = (*scope)->array[top];
	ICalErrorCode status = OK;

	/** 
	 * Checks if newScope can only occur from a currentScope.
	 * So, VEVENT can only occur in VCALENDAR.
	 * VALARM can only occur in VEVENT.
	 * Other than that it is an error, referenced to current scope. 
	 */
	if (currentScope == VCALENDAR && newScope != VEVENT) status = INV_CAL;

	else if (currentScope == VEVENT && newScope != VALARM) status = INV_EVENT;

	else if (currentScope == VALARM) status = INV_ALARM;

	if (status == OK) {
		(*scope)->top++;
		(*scope)->array[ (*scope)->top ] = newScope;
	}

	return status;
}

/* Pop function to determine previous scope and prevent errors. */
ICalErrorCode pop(Stack** scope, const int scopeType) {
	int top = (*scope)->top;
	int currentScope = (*scope)->array[top];
	/*Checks if current scope is the same scope that it is returning from, else print scope specific error*/
	if (currentScope == scopeType) (*scope)->top--;
	else return scopeSpecificError(currentScope);

	return OK;
}

/**
 * Parses line depending on colon or semi-colon.
 * Returns position of colon or semi-colon, else returns NULL.
 */
char* parseLine(char* line, ICalErrorCode* status) {
	char * pos;

	/* checks if property name is missing */
	if (line[0] == ':') {
		*status = OTHER_ERROR;
		return NULL;
	}

	/*if pos != NULL, then property name is to the right of the semi-colon, else its delimited by a colon*/
	if ((pos = colonOrSemi(line)) != NULL) {
		pos++;
		if (*pos != '\0') { /* checks if value is missing */
			pos--;
		    return pos;
		}
	}

	*status = OTHER_ERROR;
	return NULL;
}

/*Finds the very first occurence of a colon or Semi colon*/
char* colonOrSemi(char* line) {
	bool semicolon = false;;
	char* temp = line;
	
	while(*temp != '\0') {
		if(*temp == ':')
			semicolon = true;
		temp++;
	}

	if(!semicolon) return NULL;

	while (*line != '\0') {
		if (*line == ':') return line;
		if (*line == ';') return line;
		line++;
	}
	return NULL;
}

/*compares two strings ignoring case*/
bool strCaseCmp(const char* a, const char* b) {
	if (strlen(a) != strlen(b)) return false;

	while (*a && *b) {
		if ( upperCase(*a) != upperCase(*b) ) return false;
		a++;
		b++;
	}
	return true;
}

/* Compares two strings for a maximum number of characters, ignoring case */
bool strnCaseCmp(const char* a, const char* b, int bytes) {
	int i = 0;
	if (i >= bytes) return false;

	while (i < bytes && ( *a && *b )) {
		if (upperCase(*a) != upperCase(*b)) return false;
		a++;
		b++;
	}
	return true;
}

/* This function lower cases the character if it is uppercase  */
char upperCase(char c) {
	return (c >= 97 && c <= 122) ? c -= 32 : c;
}

/* Check file if it's valid. */
ICalErrorCode checkFile(FILE** fp, char* fileName) {
	if (fileName == NULL) return INV_FILE; 	/*invalid if file is null*/

	/*if file does not have .ics ending then invalid*/
	if (checkFileExtension(fileName) == INV_FILE) return INV_FILE;

	/*check if files exists*/
	if ((*fp = fopen(fileName, "r")) == NULL) {
		return INV_FILE;
	}
	else { /*Check if file is not empty*/
		fseek(*fp, 0, SEEK_END);
		if (ftell(*fp) == 0) {
			fclose(*fp);
			return INV_FILE;
		}
		fclose(*fp);
	}

	if ((*fp = fopen(fileName, "r")) == NULL) return INV_FILE;
	return OK;
}

/* Check file extenion, only valid if it is .ics */
ICalErrorCode checkFileExtension(char* fileName) {
	char* pos = NULL;
	int i = 0;
	for(i = strlen(fileName) - 1; i >=0; i--) {
		if (fileName[i] == '.') {
			pos = fileName + i;
			break;
		}
	}
	if (pos && strcmp(".ics", pos) == 0) return OK;
	return INV_FILE;
}

/* isDigit, checks if every element is a number. */
bool isDigit(const char* num) {
	int i = 0;
	int len = strlen(num);
	while( i < len ) {
		if (*num < 48 || *num > 57) return false;
		i++;
		num++;
	}
	return true;
}

/* Free ptr */
void freePtr(char** ptr) {
	if (ptr != NULL) free(*ptr);
	*ptr = NULL;
}

/* Checks if a string is empty. */
bool isEmpty(const char* s) {
	return (s == NULL || s[0] == '\0') ? true : false;
}

/* Checks if a static array is valid - contains a null terminator, 
if it doesn't means that the content size was originally to large for the array */
bool isStaticArrayInvalid(const char* s, int size) {
	int i = 0;
	bool null = false;
	for(i = 0; i < size; i++) {
		if((s[i]) == '\0') {
			null = true;
			break;
		} 
	}
	if(null && s[0] != '\0' ) return false;
	return true;
}



