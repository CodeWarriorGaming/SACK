#include <stdhdrs.h>

#define JSON_EMITTER_SOURCE
#include <json_emitter.h>

#include "json.h"


#ifdef __cplusplus
SACK_NAMESPACE namespace network { namespace json {
#endif


enum json_parse_state{
	JSON_PARSE_STATE_PICK_WHATS_NEXT       = 0x01
      , JSON_PARSE_STATE_GET_NUMBER       = 0x02
      , JSON_PARSE_STATE_GET_NAME         = 0x04
      , JSON_PARSE_STATE_GET_STRING       = 0x08
      , JSON_PARSE_STATE_GET_ARRAY_MEMBER = 0x10
};

enum word_char_states {
	WORD_POS_RESET = 0,
	WORD_POS_TRUE_1,
	WORD_POS_TRUE_2,
	WORD_POS_TRUE_3,
	WORD_POS_TRUE_4,
	WORD_POS_FALSE_1, // 11
	WORD_POS_FALSE_2,
	WORD_POS_FALSE_3,
	WORD_POS_FALSE_4,
	WORD_POS_NULL_1, // 21  get u
	WORD_POS_NULL_2, //  get l
	WORD_POS_NULL_3, //  get l
	WORD_POS_UNDEFINED_1,  // 31 
	WORD_POS_UNDEFINED_2,
	WORD_POS_UNDEFINED_3,
	WORD_POS_UNDEFINED_4,
	WORD_POS_UNDEFINED_5,
	WORD_POS_UNDEFINED_6,
	WORD_POS_UNDEFINED_7,
	WORD_POS_UNDEFINED_8,
	//WORD_POS_UNDEFINED_9, // instead of stepping to this value here, go to RESET
	WORD_POS_NAN_1,
	WORD_POS_NAN_2,
	//WORD_POS_NAN_3,// instead of stepping to this value here, go to RESET
	WORD_POS_INFINITY_1,
	WORD_POS_INFINITY_2,
	WORD_POS_INFINITY_3,
	WORD_POS_INFINITY_4,
	WORD_POS_INFINITY_5,
	WORD_POS_INFINITY_6,
	WORD_POS_INFINITY_7,
	//WORD_POS_INFINITY_8,// instead of stepping to this value here, go to RESET
};

#define CONTEXT_UNKNOWN 0
#define CONTEXT_IN_ARRAY 1
#define CONTEXT_IN_OBJECT 2

struct json_parse_context {
	int context;
	PDATALIST elements;
	struct json_context_object *object;
};

#define RESET_VAL()  {  \
	val.value_type = VALUE_UNDEFINED; \
	val.contains = NULL;              \
	val.name = NULL;                  \
	val.string = NULL;                \
	negative = FALSE; }

typedef struct json_parse_context PARSE_CONTEXT, *PPARSE_CONTEXT;
#define MAXPARSE_CONTEXTSPERSET 128
DeclareSet( PARSE_CONTEXT );
PPARSE_CONTEXTSET parseContexts;

TEXTSTR json_escape_string( CTEXTSTR string ) {
	size_t n;
	size_t m = 0;
	TEXTSTR output;
	if( !string ) return NULL;
	for( n = 0; string[n]; n++ ) {
		if( string[n] == '"' )
			m++;
	}
	output = NewArray( TEXTCHAR, n+m+1 );
	m = 0;
	for( n = 0; string[n]; n++ ) {
		if( string[n] == '"' ) {
			output[m++] = '\\';
		}
		output[m++] = string[n];
	}
	return output;
}

LOGICAL json_parse_message( TEXTSTR msg
                                 , size_t msglen
                                 , PDATALIST *_msg_output )
{
	/* I guess this is a good parser */
	PDATALIST elements = NULL;
	size_t m = 0; // m is the output path; leave text inline; but escaped chars can offset/change the content

	size_t n = 0; // character index;
	size_t _n = 0; // character index; (restore1)
	int word = WORD_POS_RESET;
	TEXTRUNE c;
	LOGICAL status = TRUE;
	LOGICAL negative = FALSE;

	PLINKSTACK context_stack = NULL;

	LOGICAL first_token = TRUE;
	//enum json_parse_state state;
	PPARSE_CONTEXT context = GetFromSet( PARSE_CONTEXT, &parseContexts );
	int parse_context = CONTEXT_UNKNOWN;
	struct json_value_container val;
	int comment = 0;

	char const * msg_input = (char const *)msg;
	char const * _msg_input;
	//char *token_begin;

	if( !_msg_output )
		return FALSE;

	elements = CreateDataList( sizeof( val ) );

	val.value_type = VALUE_UNDEFINED;
	val.contains = NULL;
	val.name = NULL;
	val.string = NULL;

	while( status && ( n < msglen ) && ( c = GetUtfChar( &msg_input ) ) )
	{
		n = msg_input - msg;
		if( comment ) {
			if( comment == 1 ) {
				if( c == '*' ) { comment = 3; continue; }
				if( c != '/' ) { lprintf( WIDE("Fault while parsing; unexpected %c at %") _size_f, c, n ); status = FALSE; }
				else comment = 2;
				continue;
			}
			if( comment == 2 ) {
				if( c == '\n' ) { comment = 0; continue; }
				else continue;
			}
			if( comment == 3 ){
				if( c == '*' ) { comment = 4; continue; }
				else continue;
			}
			if( comment == 4 ) {
				if( c == '/' ) { comment = 0; continue; }
				else { if( c != '*' ) comment = 3; continue; }
			}
		}
		switch( c )
		{
		case '/':
			if( !comment ) comment = 1;
			break;
		case '{':
			{
				struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &parseContexts );
				val.value_type = VALUE_OBJECT;
				val.contains = CreateDataList( sizeof( val ) );

				AddDataItem( &elements, &val );

				old_context->context = parse_context;
				old_context->elements = elements;
				elements = val.contains;
				PushLink( &context_stack, old_context );
				RESET_VAL();
				parse_context = CONTEXT_IN_OBJECT;
			}
			break;

		case '[':
			{
				struct json_parse_context *old_context = GetFromSet( PARSE_CONTEXT, &parseContexts );

				val.value_type = VALUE_ARRAY;
				val.contains = CreateDataList( sizeof( val ) );
				AddDataItem( &elements, &val );

				old_context->context = parse_context;
				old_context->elements = elements;
				elements = val.contains;
				PushLink( &context_stack, old_context );

				RESET_VAL();
				parse_context = CONTEXT_IN_ARRAY;
			}
			break;

		case ':':
			if( parse_context == CONTEXT_IN_OBJECT )
			{
				if( val.name ) {
					lprintf( "two names single value?" );
				}
				val.name = val.string;
				val.string = NULL;

				val.value_type = VALUE_UNDEFINED;
			}
			else
			{
				if( parse_context == CONTEXT_IN_ARRAY )
					lprintf( WIDE("(in array, got colon out of string):parsing fault; unexpected %c at %") _size_f, c, n );
				else
					lprintf( WIDE("(outside any object, got colon out of string):parsing fault; unexpected %c at %") _size_f, c, n );
				status = FALSE;
			}
			break;
		case '}':
			if( parse_context == CONTEXT_IN_OBJECT )
			{
				// first, add the last value
				if( val.value_type != VALUE_UNDEFINED ) {
					AddDataItem( &elements, &val );
				}
				RESET_VAL();

				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt-1 );
					oldVal->contains = elements;  // save updated elements list in the old value in the last pushed list.
					
					parse_context = old_context->context;
					elements = old_context->elements;
					DeleteFromSet( PARSE_CONTEXT, parseContexts, old_context );

				}
				//n++;
			}
			else
			{
				lprintf( WIDE("Fault while parsing; unexpected %c at %") _size_f, c, n );
			}
			break;
		case ']':
			if( parse_context == CONTEXT_IN_ARRAY )
			{
				if( val.value_type != VALUE_UNDEFINED ) {
					AddDataItem( &elements, &val );
				}
				RESET_VAL();
				{
					struct json_parse_context *old_context = (struct json_parse_context *)PopLink( &context_stack );
					struct json_value_container *oldVal = (struct json_value_container *)GetDataItem( &old_context->elements, old_context->elements->Cnt-1 );
					oldVal->contains = elements;  // save updated elements list in the old value in the last pushed list.
					
					parse_context = old_context->context;
					elements = old_context->elements;
					DeleteFromSet( PARSE_CONTEXT, parseContexts, old_context );
				}
			}
			else
			{
				lprintf( WIDE("bad context %d; fault while parsing; '%c' unexpected at %") _size_f, parse_context, c, n );// fault
			}
			break;
		case ',':
			if( ( parse_context == CONTEXT_IN_ARRAY ) 
			   || ( parse_context == CONTEXT_IN_OBJECT ) )
			{
				if( val.value_type != VALUE_UNDEFINED ) {
					AddDataItem( &elements, &val );
				}
				RESET_VAL();
			}
			else
			{
				lprintf( WIDE("bad context; fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
			}
			break;

		default:
			switch( c )
			{
			case '"':
			case '\'':
				{
					// collect a string
					int escape = 0;
					TEXTRUNE start_c = c;
					val.string = msg + m;
					while( (_n=n), (( n < msglen ) && (c = GetUtfChar( &msg_input ) )) )
					{
						if( c == '\\' )
						{
							if( escape ) msg[m++] = '\\';
							else escape = 1;
						}
						else if( ( c == '"' ) || ( c == '\'' ) )
						{
							if( escape ) { msg[m++] = c; escape = FALSE; }
							else if( c == start_c ) {
								//AddDataItem( &elements, &val );
								//RESET_VAL();
								break;
							} else msg[m++] = c; // other else is not valid close quote; just store as content.
						}
						else
						{
							if( escape )
							{
								switch( c )
								{
								case '\r':
									continue;
								case '\n':
									escape = FALSE;
									continue;
								case '/':
								case '\\':
								case '"':
									msg[m++] = c;
									break;
								case 't':
									msg[m++] = '\t';
									break;
								case 'b':
									msg[m++] = '\b';
									break;
								case 'n':
									msg[m++] = '\n';
									break;
								case 'r':
									msg[m++] = '\r';
									break;
								case 'f':
									msg[m++] = '\f';
									break;
								case 'u':
									{
										TEXTRUNE hex_char = 0;
										int ofs;
										for( ofs = 0; ofs < 4; ofs++ )
										{
											c = GetUtfChar( &msg_input );
											hex_char *= 16;
											if( c >= '0' && c <= '9' )      hex_char += c - '0';
											else if( c >= 'A' && c <= 'F' ) hex_char += ( c - 'A' ) + 10;
											else if( c >= 'a' && c <= 'f' ) hex_char += ( c - 'F' ) + 10;
											else
												lprintf( WIDE("(escaped character, parsing hex of \\u) fault while parsing; '%c' unexpected at %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
														 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
														 , msg + n - ( (n>3)?3:n )
														 , c
														 , msg + n + 1
														 );// fault
										}
										m += ConvertToUTF8( msg + m, hex_char );
									}
									break;
								default:
									lprintf( WIDE("(escaped character) fault while parsing; '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
											 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
											 , msg + n - ( (n>3)?3:n )
											 , c
											 , msg + n + 1
											 );// fault
									break;
								}
								escape = 0;
							}
							else {
								m += ConvertToUTF8( msg + m, c );
							}
						}
					}
					msg[m++] = 0;  // terminate the string.
					val.value_type = VALUE_STRING;
					break;
				}

			case ' ':
			case '\t':
			case '\r':
			case '\n':
				// skip whitespace
				//n++;
				//lprintf( "whitespace skip..." );
				break;

		//----------------------------------------------------------
		//  catch characters for true/false/null/undefined which are values outside of quotes
			case 't':
				if( word == WORD_POS_RESET ) word = WORD_POS_TRUE_1;
				else if( word == WORD_POS_INFINITY_6 ) word = WORD_POS_INFINITY_7;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'r':
				if( word == WORD_POS_TRUE_1 ) word = WORD_POS_TRUE_2;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'u':
				if( word == WORD_POS_TRUE_2 ) word = WORD_POS_TRUE_3;
				else if( word == WORD_POS_NULL_1 ) word = WORD_POS_NULL_2;
				else if( word == WORD_POS_RESET ) word = WORD_POS_UNDEFINED_1;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'e':
				if( word == WORD_POS_TRUE_3 ) {
					val.value_type = VALUE_TRUE;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_4 ) {
					val.value_type = VALUE_FALSE;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_UNDEFINED_3 ) word = WORD_POS_UNDEFINED_4;
				else if( word == WORD_POS_UNDEFINED_7 ) word = WORD_POS_UNDEFINED_8;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'n':
				if( word == WORD_POS_RESET ) word = WORD_POS_NULL_1;
				else if( word == WORD_POS_UNDEFINED_1 ) word = WORD_POS_UNDEFINED_2;
				else if( word == WORD_POS_UNDEFINED_6 ) word = WORD_POS_UNDEFINED_7;
				else if( word == WORD_POS_INFINITY_1 ) word = WORD_POS_INFINITY_2;
				else if( word == WORD_POS_INFINITY_5 ) word = WORD_POS_INFINITY_6;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'd':
				if( word == WORD_POS_UNDEFINED_2 ) word = WORD_POS_UNDEFINED_3;
				else if( word == WORD_POS_UNDEFINED_8 ) { val.value_type=VALUE_UNDEFINED; word = WORD_POS_RESET; }
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'i':
				if( word == WORD_POS_UNDEFINED_5 ) word = WORD_POS_UNDEFINED_6;
				else if( word == WORD_POS_INFINITY_3 ) word = WORD_POS_INFINITY_4;
				else if( word == WORD_POS_INFINITY_5 ) word = WORD_POS_INFINITY_6;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'l':
				if( word == WORD_POS_NULL_2 ) word = WORD_POS_NULL_3;
				else if( word == WORD_POS_NULL_3 ) {
					val.value_type = VALUE_NULL;
					word = WORD_POS_RESET;
				} else if( word == WORD_POS_FALSE_2 ) word = WORD_POS_FALSE_3;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'f':
				if( word == WORD_POS_RESET ) word = WORD_POS_FALSE_1;
				else if( word == WORD_POS_UNDEFINED_4 ) word = WORD_POS_UNDEFINED_5;
				else if( word == WORD_POS_INFINITY_2 ) word = WORD_POS_INFINITY_3;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'a':
				if( word == WORD_POS_FALSE_1 ) word = WORD_POS_FALSE_2;
				else if( word == WORD_POS_NAN_1 ) word = WORD_POS_NAN_2;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 's':
				if( word == WORD_POS_FALSE_3 ) word = WORD_POS_FALSE_4;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'I':
				if( word == WORD_POS_RESET ) word = WORD_POS_INFINITY_1;
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'N':
				if( word == WORD_POS_RESET ) word = WORD_POS_NAN_1;
				else if( word == WORD_POS_NAN_2 ) { val.value_type = negative ? VALUE_NEG_NAN : VALUE_NAN; word = WORD_POS_RESET; }
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
			case 'y':
				if( word == WORD_POS_INFINITY_7 ) { val.value_type = negative ? VALUE_NEG_INFINITY : VALUE_INFINITY; word = WORD_POS_RESET; }
				else lprintf( WIDE("fault while parsing; '%c' unexpected at %") _size_f, c, n );// fault
				break;
		//
 	 	//----------------------------------------------------------
			case '-':
				negative = !negative;
				break;

			default:
				if( ( c >= '0' && c <= '9' ) || ( c == '+' ) )
				{
					LOGICAL fromHex;
					fromHex = FALSE;
					// always reset this here....
					// keep it set to determine what sort of value is ready.
					val.float_result = 0;

					val.string = msg + m;
					msg[m++] = c;  // terminate the string.
					while( (_msg_input=msg_input),(( n < msglen ) && (c = GetUtfChar( &msg_input )) ) )
					{
						n = (msg_input - msg );
						// leading zeros should be forbidden.
						if( ( c >= '0' && c <= '9' )
							|| ( c == '-' )
							|| ( c == '+' )
						  )
						{
							msg[m++] = c;
						}
						else if( c == 'x' ) {
							// hex conversion.
							if( !fromHex ) {
								fromHex = TRUE;
								msg[m++] = c;
							}
							else {
								lprintf( WIDE("fault wile parsing; '%c' unexpected at %") _size_f, c, n );
								break;
							}
						}
						else if( ( c =='e' ) || ( c == 'E' ) || ( c == '.' ) )
						{
							val.float_result = 1;
							msg[m++] = c;
						}
						else
						{
							break;
						}
					}
					{
						msg[m++] = 0;

						if( val.float_result )
						{
							CTEXTSTR endpos;
							val.result_d = FloatCreateFromText( val.string, &endpos );
							if( negative ) { val.result_d = -val.result_d; negative = FALSE; }
						}
						else
						{
							val.result_n = IntCreateFromText( val.string );
							if( negative ) { val.result_n = -val.result_n; negative = FALSE; }
						}
					}
					msg_input = _msg_input;
					n = msg_input - msg;
					val.value_type = VALUE_NUMBER;
				}
				else
				{
					// fault, illegal characer (whitespace?)
					lprintf( WIDE("fault parsing '%c' unexpected %")_size_f WIDE(" (near %*.*s[%c]%s)"), c, n
							 , (int)( (n>3)?3:n ), (int)( (n>3)?3:n )
							 , msg + n - ( (n>3)?3:n )
							 , c
							 , msg + n + 1
							 );// fault
				}
				break; // default
			}
			break; // default of high level switch
		}
	}

	{
		struct json_parse_context *old_context;
		while( ( old_context = (struct json_parse_context *)PopLink( &context_stack ) ) ) {
			lprintf( "warning unclosed contexts...." );
			DeleteFromSet( PARSE_CONTEXT, parseContexts, old_context );
		}
		if( context_stack )
			DeleteLinkStack( &context_stack );
	}

	if( val.value_type != VALUE_UNDEFINED ) 
		AddDataItem( &elements, &val );

	(*_msg_output) = elements;
	// clean all contexts

	if( !status )
	{
		//Release( (*_msg_output) );
		//(*_msg_output) = NULL;
	}
	return status;
}

void json_dispose_decoded_message( struct json_context_object *format
                                 , POINTER msg_data )
{
	// a complex format might have sub-parts .... but for now we'll assume simple flat structures
	Release( msg_data );
}

void json_dispose_message( PDATALIST *msg_data )
{
	struct json_value_container *val;
	INDEX idx;
	DATA_FORALL( (*msg_data), idx, struct json_value_container*, val )
	{
		//if( val->name ) Release( val->name );
		//if( val->string ) Release( val->string );
		if( val->value_type == VALUE_OBJECT )
			json_dispose_message( &val->contains );
	}
	// quick method
	DeleteDataList( msg_data );

}


// puts the current collected value into the element; assumes conversion was correct
static void FillDataToElement( struct json_context_object_element *element
							    , size_t object_offset
								, struct json_value_container *val
								, POINTER msg_output )
{
	if( !val->name )
		return;
	// remove name; indicate that the value has been used.
	Release( val->name );
	val->name = NULL;
	switch( element->type )
	{
	case JSON_Element_String:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_NULL:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = NULL;
				break;
			case VALUE_STRING:
				((CTEXTSTR*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = StrDup( val->string );
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->value_type );
				break;
			}
		}
		break;
	case JSON_Element_Integer_64:
	case JSON_Element_Integer_32:
	case JSON_Element_Integer_16:
	case JSON_Element_Integer_8:
	case JSON_Element_Unsigned_Integer_64:
	case JSON_Element_Unsigned_Integer_32:
	case JSON_Element_Unsigned_Integer_16:
	case JSON_Element_Unsigned_Integer_8:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_TRUE:
				switch( element->type )
				{
				case JSON_Element_String:
				case JSON_Element_CharArray:
				case JSON_Element_Float:
				case JSON_Element_Double:
				case JSON_Element_Array:
				case JSON_Element_Object:
				case JSON_Element_ObjectPointer:
				case JSON_Element_List:
				case JSON_Element_Text:
				case JSON_Element_PTRSZVAL:
				case JSON_Element_PTRSZVAL_BLANK_0:
				case JSON_Element_UserRoutine:
				case JSON_Element_Raw_Object:
					lprintf( "Uhandled element conversion." );
					break;

				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((int8_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((int16_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((int32_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 1;
					break;
				}
				break;
			case VALUE_FALSE:
				switch( element->type )
				{
				case JSON_Element_String:
				case JSON_Element_CharArray:
				case JSON_Element_Float:
				case JSON_Element_Double:
				case JSON_Element_Array:
				case JSON_Element_Object:
				case JSON_Element_ObjectPointer:
				case JSON_Element_List:
				case JSON_Element_Text:
				case JSON_Element_PTRSZVAL:
				case JSON_Element_PTRSZVAL_BLANK_0:
				case JSON_Element_UserRoutine:
				case JSON_Element_Raw_Object:
					lprintf( "Uhandled element conversion." );
					break;

				case JSON_Element_Integer_64:
				case JSON_Element_Unsigned_Integer_64:
					((int8_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				case JSON_Element_Integer_32:
				case JSON_Element_Unsigned_Integer_32:
					((int16_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				case JSON_Element_Integer_16:
				case JSON_Element_Unsigned_Integer_16:
					((int32_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				case JSON_Element_Integer_8:
				case JSON_Element_Unsigned_Integer_8:
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = 0;
					break;
				}
				break;
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int") );
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (int64_t)val->result_d;
				}
				else
				{
					((int64_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = val->result_n;
				}
				break;
			default:
				lprintf( WIDE("Expected a string, but parsed result was a %d"), val->value_type );
				break;
			}
		}
		break;

	case JSON_Element_Float:
	case JSON_Element_Double:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_NUMBER:
				if( val->float_result )
				{
					if( element->type == JSON_Element_Float )
						((float*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (float)val->result_d;
					else
						((double*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = val->result_d;

				}
				else
				{
               // this is probably common (0 for instance)
					lprintf( WIDE("warning received int, converting to float") );
					if( element->type == JSON_Element_Float )
						((float*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (float)val->result_n;
					else
						((double*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (double)val->result_n;

				}
				break;
			default:
				lprintf( WIDE("Expected a float, but parsed result was a %d"), val->value_type );
				break;
			}

		}

		break;
	case JSON_Element_PTRSZVAL_BLANK_0:
	case JSON_Element_PTRSZVAL:
		if( element->count )
		{
		}
		else if( element->count_offset != JSON_NO_OFFSET )
		{
		}
		else
		{
			switch( val->value_type )
			{
			case VALUE_NUMBER:
				if( val->float_result )
				{
					lprintf( WIDE("warning received float, converting to int (uintptr_t)") );
					((uintptr_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (uintptr_t)val->result_d;
				}
				else
				{
					// this is probably common (0 for instance)
					((uintptr_t*)( ((uintptr_t)msg_output) + element->offset + object_offset ))[0] = (uintptr_t)val->result_n;
				}
				break;
			}
		}
		break;
	}
}


LOGICAL json_decode_message( struct json_context *format
								, PDATALIST msg_data
								, struct json_context_object **result_format
								, POINTER *_msgbuf )
{
	#if 0
	PLIST elements = format->elements;
	struct json_context_object_element *element;

	// message is allcoated +7 bytes in case last part is a uint8_t type
	// all integer stores use uint64_t type to store the collected value.
	if( !msg_output )
		msg_output = (*_msg_output)
			= NewArray( uint8_t, format->object_size + 7  );

	LOGICAL first_token = TRUE;

				if( first_token )
				{
					// begin an object.
					// content is
					elements = format->members;
					PushLink( &element_lists, elements );
					first_token = 0;
					//n++;
				}
				else
				{
					INDEX idx;
					// begin a sub object, we should have just had a name for it
					// since this will be the value of that name.
					// this will eet 'element' to NULL (fall out of loop) or the current one to store values into
					LIST_FORALL( elements, idx, struct json_context_object_element *, element )
					{
						if( StrCaseCmp( element->name, GetText( val.name ) ) == 0 )
						{
							if( ( element->type == JSON_Element_Object )
								|| ( element->type == JSON_Element_ObjectPointer ) )
							{
								if( element->object )
								{
									// save prior element list; when we return to this one?
									struct json_parse_context *old_context = New( struct json_parse_context );
									old_context->context = parse_context;
									old_context->elements = elements;
									old_context->object = format;
									PushLink( &context_stack, old_context );
									format = element->object;
									elements = element->object->members;
									//n++;
									break;
								}
								else
								{
									lprintf( WIDE("Error; object type does not have an object") );
									status = FALSE;
								}
							}
							else
							{
								lprintf( WIDE("Incompatible value expected object type, type is %d"), element->type );
							}
						}
					}
				}
#endif
	return FALSE;
}


#ifdef __cplusplus
} } SACK_NAMESPACE_END
#endif



