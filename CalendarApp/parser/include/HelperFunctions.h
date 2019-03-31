/*
	HelperFunctions.h
	Header file.
	By: Hrutvikkumar Patel (hrutvikk@uoguelph.ca)
	Student ID: 1002517
	Date: February 27, 2019
	CIS2750
	Assignment #1
*/


#ifndef HELPERFUNCTIONS_H
#define HELPERFUNCTIONS_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

typedef struct {
	int top;
	int array[];
} Stack;

typedef enum {VCALENDAR, VEVENT, VALARM} Scope;

typedef enum {NAME, VALUE} ToUpperType;

/* Parses Content lines from file. */
char* myFgets(FILE *fp, ICalErrorCode* status);
ICalErrorCode removeComments(FILE* fp);

void freePtr(char** ptr);

/* Parses a content line into property name and description. */
char* parseLine(char* line, ICalErrorCode* status);

/* Helper String and C type functions */
char* colonOrSemi(char* line);
bool strCaseCmp(const char* a, const char* b);
bool strnCaseCmp(const char* a, const char* b, int bytes);
char upperCase(char c);
bool isDigit(const char* num);

/* Initialize component function */
Event* initEvent();
Alarm* initAlarm();
ICalErrorCode initCalendarObj(Calendar** obj);


ICalErrorCode buildCalendar(FILE* fp, Calendar** obj, Stack** scope);
ICalErrorCode buildEvent( FILE* fp, Calendar** obj, Stack** scope);
ICalErrorCode buildAlarm( FILE* fp, Event* event, Stack** scope);
ICalErrorCode addDateTime(DateTime* date, char* line);
Property* buildProperty(char* line, char* pos);

/* Check if File is valid. */
ICalErrorCode checkFileExtension(char* fileName);
ICalErrorCode checkFile(FILE** fp, char* fileName);

/* Validates Requirements */
ICalErrorCode validateCalRequirements(const Calendar* obj);
ICalErrorCode validateEventRequirements(const Event* event);
ICalErrorCode validateAlarmRequirements(const Alarm* alarm);
ICalErrorCode validateEvent(const Event* event);
ICalErrorCode validateAlarm(const Alarm* alarm);

bool isEmpty(const char* s);
bool isStaticArrayInvalid(const char* s, int size);
bool isCalProp(const char* s);
bool isEventProp(const char* s);
bool isAlarmProp(const char* s);

bool OccursOnlyOncePropCounter(const char* propName, const char** properties, int* count, int size);

/* Determines parsing scope and validates if it is a valid scope transfer. */
ICalErrorCode setScope(char* line, Stack** scope, int* currentScope);
ICalErrorCode push(Stack** scope, int newScope);
ICalErrorCode pop(Stack** scope, const int currentScope);
ICalErrorCode scopeSpecificError(int currentScope);


/* Print Calendar */
ICalErrorCode writeEvent(FILE* fp, Event* event);
ICalErrorCode writeAlarm(FILE* fp, Alarm* alarm);
ICalErrorCode writeProperty(FILE* fp, Property* prop);
void freeWriteState(FILE** fp, char** buf, char** tempBuf);

// JSON functions

/* Escapes  backslashes and double quotes from a string before converting it into a JSON string */
char* escape(const char* str);

//Check if JSON string contains correct ending
bool checkJSONEnd(const char* str);

/* Converts dateTimeJSON string into Date Time Struct */
DateTime JSONtoDT(const char* str);

/* convert a property element to JSON */
char* propertyToJSON(const Property* property);

/* Converts list of events into a JSON string */
char* propertyListToJSON(const List* propList);

/* Converts propertyJSON string into a Property Struct */
Property* JSONtoProperty(const char* str);

char* alarmToJSON(const Alarm* alarm);

char* alarmListToJSON(const List* alarmList);

Alarm* JSONToALarm(const char* str);

// wrapper function for adding an event to a calendar
ICalErrorCode sharedAddEventToCalendar(char* filename, char* creationDT, char* startDT, char* UID, char* summary);

ICalErrorCode sharedCreateCalendar(char* filename, char* cal, char* creationDT, char* startDT, char* UID, char* summary);

ICalErrorCode sharedValidateUploadedFile(char* filename);

#endif	
