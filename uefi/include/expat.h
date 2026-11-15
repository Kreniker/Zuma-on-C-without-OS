#ifndef ZUMA_XML_H
#define ZUMA_XML_H
/* Resource-only XML subset, not a general-purpose Expat replacement. */
typedef struct ZumaXml *XML_Parser;
enum XML_Status { XML_STATUS_ERROR=0, XML_STATUS_OK=1 };
XML_Parser XML_ParserCreate(const char *encoding);
void XML_SetElementHandler(XML_Parser p,void (*start)(void *,const char *,const char **),void (*end)(void *,const char *));
enum XML_Status XML_Parse(XML_Parser p,const char *data,int length,int final);
int XML_GetErrorCode(XML_Parser p);
const char *XML_ErrorString(int code);
void XML_ParserFree(XML_Parser p);
#endif
