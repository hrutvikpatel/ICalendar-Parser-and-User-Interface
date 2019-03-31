/*
	JSON.c
	C file.
	By: Hrutvikkumar Patel (hrutvikk@uoguelph.ca)
	Student ID: 1002517
 	Date: March 22, 2019
	CIS2750
	Assignment #2
*/

#include "CalendarParser.h"
#include "LinkedListAPI.h"
#include "HelperFunctions.h"

/***** JSON funtions *****/

/* Escapes  backslashes and double quotes from a string before converting it into a JSON string */
char* escape(const char* str) {
	if(str == NULL) return NULL;
	int i = 0;
	int j = 0;
	int len = 0;

	while (str[i]) {
		len++;
		if(str[i] == '\"' || str[i] == '\\') len++;
		i++;
	}
	if (len == strlen(str)) return NULL;
	char* newStr = calloc(sizeof(char), len+2);

	i = 0;
	while (j <= len && i <= strlen(str)) {
		if (str[i] == '\"') {
			newStr[j++] = '\\';
			newStr[j++] = str[i++];
		}
		else if(str[i] == '\\') {
			newStr[j++] = '\\';
			newStr[j++] = str[i++];
		}
		else newStr[j++] = str[i++];
	}

	return newStr;
}

/* convert a property element to JSON */
char* propertyToJSON(const Property* property) {
    if(property == NULL) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "{}");
		return str;
	}
    Property* prop = (Property*) property;
    char* newPropDescr = escape(prop->propDescr);
    char* str = NULL;

    if( newPropDescr != NULL ) {
	    str = malloc( sizeof(char)* (strlen(newPropDescr) + strlen(prop->propName) + strlen("{\"name\":%s,\"description\":\"%s\"}") + 5));

        sprintf(str, "{\"name\":\"%s\",\"description\":\"%s\"}", 
			prop->propName, newPropDescr);
		freePtr(&newPropDescr);
    }
    else {
        str = malloc( sizeof(char)* (strlen(prop->propDescr) + strlen(prop->propName) + strlen("{\"name\":%s,\"description\":\"%s\"}") + 5));
        
        sprintf(str, "{\"name\":\"%s\",\"description\":\"%s\"}", 
			prop->propName, prop->propDescr);
    }

	return str;
}

/* Converts list of events into a JSON string */
char* propertyListToJSON(const List* propList) {
	if(propList == NULL || propList->length == 0) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "[]");
		return str;
	}

	int len = 2;
	char* str = calloc(sizeof(char), len);
	Property* property;
	List* list = (List*) propList;
	ListIterator properties = createIterator(list);
	strcpy(str, "[\0");

	/* Add events to string */
	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		char* propString = propertyToJSON(property);
		str = realloc(str, sizeof(char)*(len += strlen(propString) + 3));
		strcat(str, propString);
		strcat(str, ",");
		free(propString);
	}
	/* overwrite last ',' with ']' */
	str[strlen(str)-1] = ']';

	return str;
}

/* Converts DateTime struct into a JSON string. */
char* dtToJSON(DateTime prop) {
	char* str;
	int size = strlen("{\"date\":\"\",\"time\":\"\",\"UTC\":}") + strlen(prop.date) + strlen(prop.time) + strlen((prop.UTC == true ? "true" : "false")) + 1 ;
	str = calloc(sizeof(char), size );
	sprintf(str, "{\"date\":\"%s\",\"time\":\"%s\",\"UTC\":%s}", prop.date, prop.time, (prop.UTC == true ? "true" : "false") );	
	return str;
}

char* alarmToJSON(const Alarm* alarm) {
	if(alarm == NULL) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "{}");
		return str;
	}
	char* properties = propertyListToJSON(alarm->properties);

	int len = strlen("{\"action\":\"%s\",\"trigger\":\"%s\",\"numProps\":%d,\"properties\":%s}") + strlen(alarm->action)
        + strlen(alarm->trigger) + strlen(properties) + 5;
	char* str = calloc(sizeof(char), len);

    sprintf(str, "{\"action\":\"%s\",\"trigger\":\"%s\",\"numProps\":%d,\"properties\":%s}", 
				alarm->action, alarm->trigger, alarm->properties->length + 2, properties );

    freePtr(&properties);
	return str;
}

char* alarmListToJSON(const List* alarmList) {
	if(alarmList == NULL || alarmList->length == 0) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "[]");
		return str;
	}

	int len = 2;
	char* str = calloc(sizeof(char), len);
	Alarm* alarm;
	List* list = (List*) alarmList;
	ListIterator alarms = createIterator(list);
	strcpy(str, "[\0");

	/* Add events to string */
	while ( (alarm = (Alarm*) nextElement(&alarms)) != NULL ) {
		char* alarmString = alarmToJSON(alarm);
		str = realloc(str, sizeof(char)*(len += strlen(alarmString) + 3));
		strcat(str, alarmString);
		strcat(str, ",");
		freePtr(&alarmString);
	}
	/* overwrite last ',' with ']' */
	str[strlen(str)-1] = ']';

	return str;
}

/* Converts Event struct into a JSON string. */
char* eventToJSON(const Event* event) {
	if(event == NULL) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "{}");
		return str;
	}
    char* creationDateTime = dtToJSON(event->startDateTime);
	char* startDateTime = dtToJSON(event->startDateTime);
    char* propList = propertyListToJSON(event->properties);
    char* alarms = alarmListToJSON(event->alarms);
	char* newSummary = NULL;

	int len = strlen("{\"UID\":\"%s\",\"creationDateTime\":%s,\"startDateTime\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"%s\",\"properties\":%s,\"alarms\":%s}") 
        + strlen(event->UID) + strlen(startDateTime) + strlen(creationDateTime) + strlen(propList) + strlen(alarms) + 5;
	char* str = calloc(sizeof(char), len);


	/* Find if summary property exists and print it's description if it exists */
	bool summary = false;
	Property* property;
	ListIterator properties = createIterator(event->properties);
	while ( (property = (Property*) nextElement(&properties)) != NULL ) {
		if(strCaseCmp("SUMMARY", property->propName)) {
			summary = true;
			if ( (newSummary = escape(property->propDescr)) != NULL ) len += strlen(newSummary);
			else len += strlen(property->propDescr);

			str = realloc(str, sizeof(char)*(len));
			break;
		}
	}

	if(summary){
		if(newSummary != NULL) {
			sprintf(str, "{\"UID\":\"%s\",\"creationDateTime\":%s,\"startDateTime\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"%s\",\"properties\":%s,\"alarms\":%s}", 
			    event->UID, creationDateTime, startDateTime, event->properties->length + 3, event->alarms->length, newSummary, propList, alarms );
			freePtr(&newSummary);
		}
		else sprintf(str, "{\"UID\":\"%s\",\"creationDateTime\":%s,\"startDateTime\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"%s\",\"properties\":%s,\"alarms\":%s}", 
			event->UID, creationDateTime, startDateTime, event->properties->length + 3, event->alarms->length, property->propDescr, propList, alarms );
	}
	else {
		sprintf(str, "{\"UID\":\"%s\",\"creationDateTime\":%s,\"startDateTime\":%s,\"numProps\":%d,\"numAlarms\":%d,\"summary\":\"\",\"properties\":%s,\"alarms\":%s}", 
			event->UID, creationDateTime, startDateTime, event->properties->length + 3, event->alarms->length, propList, alarms );
	}

    freePtr(&creationDateTime);
	freePtr(&startDateTime);
    freePtr(&propList);
	freePtr(&alarms);

	return str;
}

// /* Converts list of events into a JSON string */
char* eventListToJSON(const List* eventList) {
	if(eventList == NULL || eventList->length == 0) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "[]");
		return str;
	}

	int len = 2;
	char* str = calloc(sizeof(char), len);
	Event* event;
	List* list = (List*) eventList;
	ListIterator events = createIterator(list);
	strcpy(str, "[\0");

	/* Add events to string */
	while ( (event = (Event*) nextElement(&events)) != NULL ) {
		char* evtString = eventToJSON(event);
		str = realloc(str, sizeof(char)*(len += strlen(evtString) + 3));
		strcat(str, evtString);
		strcat(str, ",");
		free(evtString);
	}
	/* overwrite last ',' with ']' */
	str[strlen(str)-1] = ']';

	return str;
}

/* Converts calendar object into a JSON string */
char* calendarToJSON(const Calendar* cal) {
	if(cal == NULL || cal->events == NULL || cal->events->length == 0) {
		char* str = calloc(sizeof(char), 3);
		strcpy(str, "{}");
		return str;
	}
	int version = (int) cal->version;
	char* prodID = escape(cal->prodID);
    char* eventList = eventListToJSON(cal->events);
    char* propList = propertyListToJSON(cal->properties);
    char* str = NULL;

    if( prodID != NULL ) {
        str = calloc(sizeof(char), strlen("{\"version\":%d,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d,\"events\":%s,\"properties\":%s}") 
        + strlen(prodID) + strlen(eventList) + strlen(propList) + 100 );

        sprintf(str, "{\"version\":%d,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d,\"events\":%s,\"properties\":%s}", 
			version, prodID, cal->properties->length + 2, cal->events->length, eventList, propList);
		freePtr(&prodID);
    }
    else {
        str = calloc(sizeof(char), strlen("{\"version\":%d,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d,\"events\":%s,\"properties\":%s}") 
        + strlen(cal->prodID) + strlen(eventList) + strlen(propList) + 100);

        		sprintf(str, "{\"version\":%d,\"prodID\":\"%s\",\"numProps\":%d,\"numEvents\":%d,\"events\":%s,\"properties\":%s}", 
			version, cal->prodID, cal->properties->length + 2, cal->events->length, eventList, propList);
    }

    freePtr(&eventList);
    freePtr(&propList);

	return str;
}

/* Check if JSON string contains correct ending */
bool checkJSONEnd(const char* str) {
    if( str[strlen(str)-2] != '\"' || str[strlen(str)-1] != '}' ) { // error check for correct JSON string ending
		return true;
	}
    return false;
}

/* Converts JSON string into a simple calender object. */
// {\"version\":
// ,\"prodID\":\"
// \"}
Calendar* JSONtoCalendar(const char* str) {
	if(isEmpty(str)) return NULL;
	Calendar* cal;
	if(initCalendarObj(&cal) != OK) return NULL;

	if( checkJSONEnd(str) ) { // error check for correct JSON string ending
		deleteCalendar(cal);
		return NULL;
	}

	char* json = (char*) str;
	char* temp1;
	char* temp2;
	char version[100];
	int bytes;

	// shift temp1 to get date value and get temp2 to be the stuff after date value
	temp1 = json + strlen("{\"version\":");
	temp2 = strstr(json, ",\"prodID\":\"");
	bytes = temp2 - temp1; // this gets the length of date
	strncpy(version, temp1, bytes);
	version[bytes] = '\0';

	sscanf(version, "%f", &(cal->version));

	// shift temp1 to get time value and get temp2 to be the stuff after time value
	temp1 = temp2 + strlen(",\"prodID\":\"");
	temp2 = strstr(json, "\"}");
	bytes = temp2 - temp1; // this gets the length of date
	strncpy(cal->prodID, temp1, bytes);
	cal->prodID[bytes] = '\0';

	return cal;
}

/* Converts JSON string into a simple event object. */
// {\"UID\":\"
// \"}
Event* JSONtoEvent(const char* str) {
	if(isEmpty(str)) return NULL;
	Event* event = initEvent();

	if( checkJSONEnd(str) ) { // error check for correct JSON string ending
		deleteEvent(event);
		return NULL;
	}

	char* json = (char*) str;
	char* temp1;
	char* temp2;
	int bytes;

	// shift temp1 to get UID value and get temp2 to be the stuff after UIDvalue
	temp1 = json + strlen("{\"UID\":\"");
	temp2 = strstr(json, "\"}");
	bytes = temp2 - temp1; // this gets the length of date
	strncpy(event->UID, temp1, bytes);
	event->UID[bytes] = '\0';

	return event;
}

/* Converts dateTimeJSON string into Date Time Struct */
	// {\"date\":\"
	// \",\"time\":\"
	// \",\"UTC\":
	// }"
DateTime JSONtoDT(const char* str) {
	DateTime dt;
	dt.UTC = true;
	strcpy(dt.date, "00000000");
	strcpy(dt.time, "000000");

	if(isEmpty(str)) return dt;

	char* json = (char*) str;
	char* temp1;
	char* temp2;
	char UTC[10];
	int bytes;

	// shift temp1 to get date value and get temp2 to be the stuff after date value
	temp1 = json + strlen("{\"date\":\"");
	temp2 = strstr(json, "\",\"time\":\"");
	bytes = temp2 - temp1; // this gets the length of date
	strncpy(dt.date, temp1, bytes);
	dt.date[bytes] = '\0';

	// shift temp1 to get time value and get temp2 to be the stuff after time value
	temp1 = temp2 + strlen("\",\"time\":\"");
	temp2 = strstr(json, "\",\"UTC\":");
	bytes = temp2 - temp1; // this gets the length of date
	strncpy(dt.time, temp1, bytes);
	dt.time[bytes] = '\0';

	// shift temp1 to get UTC value and get temp2 to be the stuff after UTC value
	temp1 = temp2 + strlen("\",\"UTC\":");
	temp2 = strstr(json, "}");
	bytes = temp2 - temp1; // this gets the length of date
	strncpy(UTC, temp1, bytes);
	UTC[bytes] = '\0';

	dt.UTC = strcmp(UTC, "true") == 0 ? true : false;

	return dt;
}

/* Converts propertyJSON string into a Property Struct */
Property* JSONtoProperty(const char* str) {
    if(isEmpty(str)) return NULL;
	if( checkJSONEnd(str) ) return NULL;
	// {\"propName\":\"
	// \",\"propDescr\":\" 
	// \"}
	char* json = (char*) str;
	char* temp1;
	char* temp2; 
	int bytes;
	Property* prop = malloc(sizeof(Property) + sizeof(char)*strlen(json));

	temp1 = json + strlen("{\"propName\":\"");
	temp2 = strstr(temp1, "\",\"propDescr\":\"");
	bytes = temp2 - temp1;

	strncpy(prop->propName, temp1, bytes);
	prop->propName[bytes] = '\0'; 
	
	temp2 += strlen("\",\"propDescr\":\"");
	temp1 = strstr(temp2, "\"}"); 
	bytes = temp1 - temp2;

	strncpy(prop->propDescr, temp2, bytes);
	prop->propDescr[bytes] = '\0'; 

	return prop;
}

ICalErrorCode sharedAddEventToCalendar(char* filename, char* creationDT, char* startDT, char* UID, char* summary) {
	Calendar* obj = NULL;
    ICalErrorCode err = createCalendar(filename, &obj);

	// if no errors in creating calendar proceed, else return error message
    if(err != OK) {
		deleteCalendar(obj);
		return err;
    }

	// create a stub event with just UID
	Event* event = JSONtoEvent(UID);

	// if summary is not empty add a summary property to event list
	if( !isEmpty(summary) ) {
		Property* prop = JSONtoProperty(summary);
		insertBack(event->properties, prop);
	}

	event->creationDateTime = JSONtoDT(creationDT);

	event->startDateTime = JSONtoDT(startDT);

	addEvent(obj, event);
	err = writeCalendar(filename, obj);

    deleteCalendar(obj);
	return err;
}

ICalErrorCode sharedCreateCalendar(char* filename, char* cal, char* creationDT, char* startDT, char* UID, char* summary) {
	Calendar* obj = JSONtoCalendar(cal);
	ICalErrorCode status = OK;

	// create a stub event with just UID
	Event* event = JSONtoEvent(UID);

	// if summary is not empty add a summary property to event list
	if( !isEmpty(summary) ) {
		Property* prop = JSONtoProperty(summary);
		insertBack(event->properties, prop);
	}

	// create calendar and add it to calendar
	event->creationDateTime = JSONtoDT(creationDT);
	event->startDateTime = JSONtoDT(startDT);
	addEvent(obj, event);

	status = validateCalendar(obj); // validate calendar

	if(status != OK) {
		deleteCalendar(obj);
		return status;
	}

	status = writeCalendar(filename, obj); // write calendar

    deleteCalendar(obj);
	return status; // return OK if all is good
}

/* Adds new event to the back of calendare event list */
void addEvent(Calendar* cal, Event* toBeAdded) {
	if( cal == NULL || toBeAdded == NULL ) return;
	insertBack(cal->events, toBeAdded);
}


/* sharedLibFunction validates file uploaded */
ICalErrorCode sharedValidateUploadedFile(char* filename) {
	Calendar* obj = NULL;
	ICalErrorCode status = OK;
	status = createCalendar(filename, &obj); // create the calendar

	if(status != OK) { // if error while creating calendar return error
		deleteCalendar(obj);
		return status;
	}

	status = validateCalendar(obj); // validate the calendar

	if(status != OK) {
		deleteCalendar(obj);
		return status;
	}

	deleteCalendar(obj);
	return status; // return OK if it made it this far
}
