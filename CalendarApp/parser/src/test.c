

#include "CalendarParser.h"
#include "HelperFunctions.h"

void test_calToJSON()
{
    Calendar *obj = NULL;
    char filename[100] = "../uploads/megaCal1.ics";
    char *error;

    ICalErrorCode err = createCalendar(filename, &obj);
    error = printError(err);
    printf("\nCreating Calender ... %s\n", error);

    if (err == OK)
    {
        freePtr(&error);
        char *json = calendarToJSON(obj);
        printf("\nCreating Calendar JSON...\n");
        printf("\n\n%s\n\n", json);
        freePtr(&json);
    }
    freePtr(&error);
    deleteCalendar(obj);
}

void test_dtToJSON()
{
    DateTime *dt = malloc(sizeof(DateTime));
    strcpy(dt->date, "19540203");
    strcpy(dt->time, "123012");
    dt->UTC = true;
    free(dtToJSON(*dt));
    dt->UTC = false;
    free(dtToJSON(*dt));
    free(dt);
}

void test_eventToJSON()
{
    char *str;
    Event *event = initEvent();

    // Test 1 from A2 Module 2
    strcpy(event->startDateTime.date, "19540203");
    strcpy(event->startDateTime.time, "123012");
    event->startDateTime.UTC = true;
    str = eventToJSON(event);
    printf("%s\n", str);
    free(str);

    // // Test 2
    strcpy(event->startDateTime.date, "20190211");
    strcpy(event->startDateTime.time, "143012");
    event->startDateTime.UTC = false;
    event->alarms->length = 2;
    char *line = calloc(sizeof(char), 100);
    strcpy(line, "SUMMARY:Do \"taxes\" and no \\drugs\\");
    ICalErrorCode status;
    char *pos = parseLine(line, &status);
    insertBack(event->properties, buildProperty(line, pos));
    str = eventToJSON(event);
    printf("%s\n", str);
    free(str);
    free(line);

    // Test 3
    deleteEvent(event);
    event = NULL;
    event = initEvent();
    strcpy(event->startDateTime.date, "19540203");
    strcpy(event->startDateTime.time, "123012");
    event->startDateTime.UTC = true;
    event->properties->length = 3;
    event->alarms->length = 0;
    str = eventToJSON(event);
    printf("%s\n", str);
    free(str);

    // Test 4 Event is NULL;
    deleteEvent(event);
    event = NULL;
    str = eventToJSON(NULL);
    printf("%s\n", str);
    free(str);
}

void test_eventListToJSON()
{
    char *str;
    Calendar *obj = NULL;
    char filename[100] = "TestFiles/Test1 EventListToJSON.ics";

    createCalendar(filename, &obj);

    // Test 1 from A2 Module 2
    str = eventListToJSON(obj->events);
    deleteCalendar(obj);
    printf("%s\n", str);
    free(str);

    // Test 2
    strcpy(filename, "TestFiles/Test2 EventListToJSON.ics");
    createCalendar(filename, &obj);
    str = eventListToJSON(obj->events);
    deleteCalendar(obj);
    printf("%s\n", str);
    free(str);

    // Test 3 Event is NULL;
    str = eventListToJSON(NULL);
    printf("%s\n", str);
    free(str);
}

void test_calendarToJSON()
{
    char *str;
    Calendar *obj = NULL;
    char filename[100] = "TestFiles/Test1 CalendarToJSON.ics";

    createCalendar(filename, &obj);

    // Test 1 from A2 Module 2
    str = calendarToJSON(obj);
    deleteCalendar(obj);
    printf("%s\n", str);
    free(str);

    // Test 2
    Calendar *obj2 = NULL;
    strcpy(filename, "TestFiles/Test2 CalendarToJSON.ics");
    createCalendar(filename, &obj2);
    str = calendarToJSON(obj2);
    deleteCalendar(obj2);
    printf("%s\n", str);
    free(str);

    // Test 3 Event is NULL;
    str = calendarToJSON(NULL);
    printf("%s\n", str);
    free(str);
}

void test_JSONtoCalendar()
{
    // Test 1
    Calendar *cal1 = JSONtoCalendar("{\"version\":2,\"prodID\":\"-//hacksw/handcal//NONSGML v1.0//EN\"}");
    if (cal1 == NULL)
        printf("NULL\n");
    else
        printf("version: %f, prodid: %s\n", cal1->version, cal1->prodID);
    deleteCalendar(cal1);

    // Test 2
    Calendar *cal2 = JSONtoCalendar(NULL);
    if (cal2 == NULL)
        printf("NULL\n");
    else
        printf("\nversion: %f, prodid: %s\n", cal2->version, cal2->prodID);
    deleteCalendar(cal2);

    // Test 3

    Calendar *cal3 = JSONtoCalendar("{\"version\":2,\"prodID\":\"-//hacksw/handcal//NONSGML v1.0//EN}");
    if (cal3 == NULL)
        printf("NULL\n");
    else
        printf("\nversion: %f, prodid: %s\n", cal3->version, cal3->prodID);
    deleteCalendar(cal3);
}

void test_JSONtoEvent()
{
    Event* event = JSONtoEvent("{\"UID\":\"Hello World.\"}");
    printf("UID: %s\n\n", event->UID);
    deleteEvent(event);
}

void test_calendarReadingWritingValidating()
{
    Calendar *obj = NULL;
    char filename[100] = "testFiles/validCalendar/testCalSimpleUTCComments.ics";
    char *error;

    ICalErrorCode err = createCalendar(filename, &obj);
    error = printError(err);
    printf("\nCreating Calender ... %s\n", error);

    if (err == OK)
    {
        freePtr(&error);
        err = validateCalendar(obj);
        error = printError(err);
        printf("\nValidating Calendar ... %s\n", error);
        freePtr(&error);
    }
    freePtr(&error);

    strcpy(filename, "TestFiles/yo.ics");
    err = writeCalendar(filename, obj);
    error = printError(err);
    printf("\nWriting out Calendar ... %s\n", error);
    freePtr(&error);

    deleteCalendar(obj);

    err = createCalendar(filename, &obj);
    error = printError(err);
    printf("\nCreating Calendar from written file ... %s\n", error);
    freePtr(&error);

    deleteCalendar(obj);
}

void test_JSONtoDT()
{
    DateTime dt = JSONtoDT("{\"date\":\"19991213\",\"time\":\"003100\",\"UTC\":false}");
    printf("date: %s\n", dt.date);
    printf("time: %s\n", dt.time);
    printf("UTC: %d\n\n", dt.UTC);
}

void test_JSONtoProperty()
{
    Property *prop = JSONtoProperty("{\"propName\":\"summary\",\"propDescr\":\"Hello World\"}\0");
    if (prop != NULL)
    {
        printf("name: %s\n", prop->propName);
        printf("description: %s\n\n", prop->propDescr);
        deleteProperty(prop);
    }
}

int main(void)
{

    // test_JSONtoDT();

    // test_JSONtoProperty();

    // test_JSONtoEvent();

    // test_JSONtoCalendar();

    return 0;
}
