/*
 * src/tutorial/email.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"
#include <stdio.h>
#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include <string.h>
#include <stdbool.h>
#include <regex.h>

PG_MODULE_MAGIC;

typedef struct emailaddr{
	char local[128];
	char domain[128];
}emailaddr;

/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/
char to_lower(char ch);
bool is_valid_str(char *str);
bool checkToken(char *str);
Datum email_in(PG_FUNCTION_ARGS);
Datum	email_out(PG_FUNCTION_ARGS);
Datum	email_receive(PG_FUNCTION_ARGS);
Datum	email_send(PG_FUNCTION_ARGS);
Datum	email_lt(PG_FUNCTION_ARGS);
Datum	email_le(PG_FUNCTION_ARGS);
Datum	email_eq(PG_FUNCTION_ARGS);
Datum	email_neq(PG_FUNCTION_ARGS);
Datum	email_gt(PG_FUNCTION_ARGS);
Datum	email_ge(PG_FUNCTION_ARGS);
Datum	email_deq(PG_FUNCTION_ARGS);
Datum	email_ndeq(PG_FUNCTION_ARGS);
Datum	email_cmp(PG_FUNCTION_ARGS);
Datum email_hval(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(email_in);
PG_FUNCTION_INFO_V1(email_out);
PG_FUNCTION_INFO_V1(email_receive);
PG_FUNCTION_INFO_V1(email_send);
PG_FUNCTION_INFO_V1(email_lt);
PG_FUNCTION_INFO_V1(email_le);
PG_FUNCTION_INFO_V1(email_eq);
PG_FUNCTION_INFO_V1(email_neq);
PG_FUNCTION_INFO_V1(email_gt);
PG_FUNCTION_INFO_V1(email_ge);
PG_FUNCTION_INFO_V1(email_deq);
PG_FUNCTION_INFO_V1(email_ndeq);
PG_FUNCTION_INFO_V1(email_cmp);
PG_FUNCTION_INFO_V1(email_hval);

static int
email_cmp_internal(emailaddr * a, emailaddr * b){
 int res1 = strcmp(a->local, b->local);
 int res2 = strcmp(a->domain, b->domain);
 if(res1 != 0)
	 return res1;
 if(res2 != 0)
	 return res2;
 return 0;
}


Datum
email_in(PG_FUNCTION_ARGS){
	char *str;
	char *front;
  char *later;
	emailaddr *result = (emailaddr*)malloc(sizeof(emailaddr));
	int i;
	str = PG_GETARG_CSTRING(0);
	for(i = 0;i<strlen(str);i++){
		str[i] = to_lower(str[i]);
	}

	if(!is_valid_str(str)){
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
				 errmsg("invalid input syntax for email: \"%s\"",
						str)));
	}

	front = strtok(str, "@");
  later = strtok(NULL, "@");
	strcpy(result->local, front);
	strcpy(result->domain, later);
	PG_RETURN_POINTER(result);
}

Datum
email_out(PG_FUNCTION_ARGS){
	emailaddr *email = (emailaddr *)PG_GETARG_POINTER(0);
	char *result = (char *)malloc(sizeof(emailaddr));
	snprintf(result, sizeof(emailaddr), "%s@%s", email->local, email->domain);
	PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Binary Input/Output functions
 *
 * These are optional.
 *****************************************************************************/

 Datum
 email_receive(PG_FUNCTION_ARGS){
 	StringInfo	buf = (StringInfo) PG_GETARG_POINTER(0);
 	emailaddr *result;

 	result = (emailaddr *) palloc(sizeof(emailaddr));
	strcpy(result->local, pq_getmsgstring(buf));
	strcpy(result->domain, pq_getmsgstring(buf));
 	PG_RETURN_POINTER(result);
 }

Datum
email_send(PG_FUNCTION_ARGS){
	emailaddr    *email = (emailaddr *) PG_GETARG_POINTER(0);
	StringInfoData buf;

	pq_begintypsend(&buf);
	pq_sendstring(&buf, email->local);
	pq_sendstring(&buf, email->domain);
	PG_RETURN_BYTEA_P(pq_endtypsend(&buf));
}

/*****************************************************************************
 * New Operators
 *
 * A practical Complex datatype would provide much more than this, of course.
 *****************************************************************************/

Datum
email_lt(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(email_cmp_internal(email1, email2) < 0);
}

Datum
email_le(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(email_cmp_internal(email1, email2) <= 0);
}

Datum
email_eq(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(email_cmp_internal(email1, email2) == 0);
}

Datum
email_neq(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(email_cmp_internal(email1, email2) != 0);
}

Datum
email_gt(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
	  PG_RETURN_BOOL(email_cmp_internal(email1, email2) > 0);
}

Datum
email_ge(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(email_cmp_internal(email1, email2) >= 0);
}

Datum
email_deq(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(strcmp(email1->domain, email2->domain) == 0);
}

Datum
email_ndeq(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_BOOL(strcmp(email1->domain, email2->domain) != 0);
}

Datum
email_cmp(PG_FUNCTION_ARGS){
		emailaddr    *email1 = (emailaddr *) PG_GETARG_POINTER(0);
		emailaddr    *email2 = (emailaddr *) PG_GETARG_POINTER(1);
		PG_RETURN_INT32(email_cmp_internal(email1, email2));
}
/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

 /**
 * Convert upper case to lower case.
 * @param ch
 * @return
 */
 char to_lower(char ch){
     if(ch >= 'A' && ch <= 'Z'){
         int in = 'a' - 'A';
         return ch + in;
     }else{
         return ch;
     }
 }

 /**
 * Is valid email format or not.
 * @param str
 * @return
 */
bool is_valid_str(char *str){
	regmatch_t p_match;
    regex_t reg;
    int i = 0;
    int count = 0;
    char strs[strlen(str)];
    bool result = true;
    char *one = ".*(\\.){2,}.*";
    int status;
    char *front;
    char *later;
    char *period;
    int period_pos = 0;
    strcpy(strs, str);

    //an email address has two parts, Local and Domain, separated by an '@' char
    for(i=0;i<strlen(str);i++){
        if(str[i] == '@'){
            count++;
        }
    }
    result &=  count > 1 ? false : true;

    //both the Local part and the Domain part consist of a sequence of Words, separated by '.' characters


    regcomp(&reg, one, REG_EXTENDED);
    status = regexec(&reg, str, 0, &p_match, 0);
    if(!(status == REG_NOMATCH)){
        result &= false;
    }

    //the Local part has one or more Words; the Domain part has two or more Words (i.e. at least one '.')
    front = strtok(strs, "@");
    later = strtok(NULL, "@");
    period = strchr(later, '.');
    period_pos = period - later;
    if(period_pos < 0 || period_pos >= strlen(later) || strlen(front) <= 0){
        result &= false;
    }

    //each Word is a sequence of one or more characters, starting with a letter
    //each Word ends with a letter or a digit
    //between the starting and ending chars, there may be any number of letters, digits or hyphens ('-')
    result &= checkToken(front);
    result &= checkToken(later);

    regfree(&reg);
    return result;
}

bool checkToken(char *str){
    bool result = true;
    regex_t reg;
    regmatch_t p_match;
    char *six = "^[a-zA-Z][a-zA-Z0-9-]*[a-zA-Z0-9]$";
    char delimer[] = ".";
    char *token = strtok(str, delimer);
    regcomp(&reg, six, REG_EXTENDED);
    while(token){
        int status_two = regexec(&reg, token, 0, &p_match, 0);
        if(status_two == REG_NOMATCH){
            if(!(strlen(token) == 1
                 && ((token[0] >= 'a' && token[0] <= 'z') || (token[0] >= 'A' && token[0] <= 'Z')))){
                result &= false;
            }
        }
        token = strtok(NULL, delimer);
    }
    regfree(&reg);
    return result;
}

Datum
email_hval(PG_FUNCTION_ARGS){
	PG_RETURN_INT32(1);
}
