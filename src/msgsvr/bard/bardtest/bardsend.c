

#include <../bard.h>

void CPROC GetAnyEvent( uintptr_t psv, char *extra )
{
	printf( "Received event with extra : %s\n", extra );
}

char *name;
void CPROC GetAnEvent( uintptr_t psv, char *extra )
{
	printf( "Received %s event with extra : %s\n", name, extra );
}



int main( int argc, char **argv )
{
	if( argc > 2 )
		BARD_RegisterForSimpleEvent( argv[2], GetAnyEvent, 0 );
   if( argc > 1 )
		BARD_IssueSimpleEvent( argv[1] );
   exit(0);
}

