


#include <stdhdrs.h>
#include <deadstart.h>
#include <procreg.h>
#include <sqlgetoption.h>


#include "netstruc.h"

//#define DEBUG_SSL_IO

#if NO_SSL

SACK_NETWORK_NAMESPACE

LOGICAL ssl_Send( PCLIENT pc, POINTER buffer, size_t length ) {
	return FALSE;
}

LOGICAL ssl_BeginServer( PCLIENT pc, POINTER cert, size_t certlen, POINTER keypair, size_t keylen, POINTER keypass, size_t keypasslen ) {
	return FALSE;
}

LOGICAL ssl_BeginClientSession( PCLIENT pc, POINTER client_keypair, size_t client_keypairlen, POINTER keypass, size_t keypasslen, POINTER rootCert, size_t rootCertLen ) {
	return FALSE;
}
SACK_NETWORK_NAMESPACE_END

#else

#define _LIB
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/err.h>

#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/pkcs12.h>

SACK_NETWORK_NAMESPACE

//#define RSA_KEY_SIZE (1024)
const int serverKBits = 4096;
const int kBits = 1024;// 4096;
const int kExp = RSA_F4;

struct internalCert {
	X509_REQ *req;
	X509 * x509;
	EVP_PKEY *pkey;
	STACK_OF( X509 ) *chain;
};


//static void gencp
struct internalCert * MakeRequest( void );


EVP_PKEY *genKey() {
	EVP_PKEY *keypair = EVP_PKEY_new();
	//int keylen;
	//char *pem_key;
	//BN_GENCB cb = { }
	BIGNUM          *bne = BN_new();
	RSA *rsa = RSA_new();
	int ret;
	ret = BN_set_word( bne, kExp );
	if( ret != 1 ) {
		BN_free( bne );
		RSA_free( rsa );
		EVP_PKEY_free( keypair );
		return NULL;
	}
	RSA_generate_key_ex( rsa, kBits, bne, NULL );
	EVP_PKEY_set1_RSA( keypair, rsa );

	BN_free( bne );
	RSA_free( rsa );

	return keypair;
}

#define ALLOW_ANON_CONNECTIONS	1

#ifndef UNICODE


struct ssl_session {
	SSL_CTX        *ctx;

	BIO *rbio;
	BIO *wbio;
	//EVP_PKEY *privkey;
	struct internalCert *cert;

	SSL            *ssl;
	uint32_t       dwOriginalFlags; // CF_CPPREAD
	cReadComplete  user_read;
	cppReadComplete  cpp_user_read;

	cNotifyCallback user_connected;
	cppNotifyCallback cpp_user_connected;

	cCloseCallback user_close;
	cppCloseCallback cpp_user_close;

	uint8_t *obuffer;
	size_t obuflen;
	uint8_t *ibuffer;
	size_t ibuflen;
	uint8_t *dbuffer;
	size_t dbuflen;
};

static struct ssl_global
{
	struct {
		BIT_FIELD bInited : 1;
	} flags;
	LOGICAL trace;
	struct tls_config *tls_config;

	SSL_CTX        *ssl_ctx_server;
	uint8_t cipherlen;
}ssl_global;




static const char *default_certs[] = {
	"Equifax_Secure_Certificate_Authority.der",
"Equifax_Secure_eBusiness_CA-1.der",
"Equifax_Secure_Global_eBusiness_CA-1.der",
"GeoTrust_CA_for_Adobe.der",
"GeoTrust_Global_CA.cer",
"GeoTrust_Global_CA.der",
"GeoTrust_Global_CA2.der",
"GeoTrust_Mobile_Device_Root_-_Privileged.der",
"GeoTrust_Mobile_Device_Root_-_Unprivileged.der",
"Geotrust_PCA_G3_Root.der",
"GeoTrust_Primary_CA.der",
"GeoTrust_Primary_CA_G2_ECC.der",
"GeoTrust_True_Credentials_CA_2.der",
"GeoTrust_Universal_CA.der",
"GeoTrust_Universal_CA2.der",
"thawte_Personal_Freemail_CA.der",
"thawte_Premium_Server_CA.der",
"thawte_Primary_Root_CA-G2_ECC.der",
"thawte_Primary_Root_CA-G3_SHA256.der",
"thawte_Primary_Root_CA.der",
"thawte_Server_CA.der" };


#if 0
static int32_t loadRsaKeys( sslKeys_t *keys, LOGICAL client )
{
	uint32_t priv_key_len = 0;
	uint32_t cert_key_len = 0;
	unsigned char *CAstream = NULL;
	int32_t CAstreamLen = 0;
	int32_t rc;
	TEXTCHAR buf[256];
	TEXTCHAR privkey_buf[256];
	TEXTCHAR cert_buf[256];
	unsigned char *priv_key = NULL;
	unsigned char *cert_key = NULL;
	FILE *file;
	size_t size;
	int fileGroup = GetFileGroup( "ssl certs", "certs" );
	{

	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		int n;
		CAstreamLen = 0;
		for( n = 0; n < ( sizeof( default_certs ) / sizeof( default_certs[0] ) ); n++ )
		{
			file = sack_fopen( fileGroup, default_certs[n], "rb" );
			if( file )
			{
				size = sack_fsize( file );
				CAstreamLen += size;
				sack_fclose( file );
			}
		}
		{
			/*
			 In-memory based keys
			 Build the CA list first for potential client auth usage
			 */
			priv_key_len = 0;
#ifndef __NO_OPTIONS__
			SACK_GetProfileString( "SSL/Private Key", "filename", "myprivkey.pem", privkey_buf, 256 );
#else
			strcpy( prikey_buf, "myprivkey.pem" );
#endif
			file = sack_fopen( fileGroup, privkey_buf, "rb" );
			if( file )
			{
				priv_key_len = sack_fsize( file );
				priv_key = NewArray( uint8_t, priv_key_len );

				sack_fread( priv_key, 1, priv_key_len, file );
				sack_fclose( file );
			}
			else
				priv_key = NULL;
		}
#ifndef __NO_OPTIONS__
		SACK_GetProfileString( "SSL/Cert Authority Extra", "filename", "mycert.pem", cert_buf, 256 );
#else
		strcpy( cer_buf, "mycert.pem" );
#endif
		file = sack_fopen( fileGroup, cert_buf, "rb" );
		if( file )
		{
			CAstreamLen += sack_fsize( file );
			sack_fclose( file );
		}

		if( CAstreamLen )
			CAstream = NewArray( uint8_t, CAstreamLen);
		CAstreamLen = 0;
		for( n = 0; n < ( sizeof( default_certs ) / sizeof( default_certs[0] ) ); n++ )
		{
			file = sack_fopen( fileGroup, default_certs[n], "rb" );
			if( file )
			{
				size = sack_fsize( file );
				CAstreamLen += sack_fread( CAstream + CAstreamLen, 1, size, file );
				//rc = matrixSslLoadRsaKeysMem(keys, NULL, 0, NULL, 0, CAstream+ CAstreamLen, size );
				//lprintf( "cert success : %d %s", rc, default_certs[n] );

				//CAstream[CAstreamLen++] = '\n';
				sack_fclose( file );
			}
		}

		file = sack_fopen( fileGroup, cert_buf, "rb" );
		if( file )
		{
			size = sack_fsize( file );
			CAstreamLen += sack_fread( CAstream + CAstreamLen, 1, size, file );
			sack_fclose( file );
		}
	}

	//if( 0 )
	{
	/*
		In-memory based keys
		Build the CA list first for potential client auth usage
	*/
		cert_key_len = 0;
#ifndef __NO_OPTIONS__
		SACK_GetProfileString( "SSL/Certificate", "filename", "mycert.pem", cert_buf, 256 );
#else
		strcpy( cert_buf, "mycert.pem" );
#endif
		file = sack_fopen( fileGroup, cert_buf, "rb" );
		if( file )
		{
			cert_key_len = sack_fsize( file );
			cert_key = NewArray( uint8_t, cert_key_len );

			sack_fread( cert_key, 1, cert_key_len, file );
			sack_fclose( file );
		}
		else
			cert_key = NULL;
	}

	if( cert_key_len || priv_key_len || CAstreamLen )
	{
		rc = matrixSslLoadRsaKeysMem(keys
			, cert_key, cert_key_len
			, priv_key, priv_key_len
			, CAstream, CAstreamLen );

		if (rc < 0) {
			lprintf("No certificate material loaded.  Exiting");
			//if (CAstream) {
			//	Deallocate(uint8_t*, CAstream);
			//}
			matrixSslDeleteKeys(keys);
			matrixSslClose();
		}
		if( cert_key_len )
			Release( cert_key );
		if( priv_key_len )
			Release( priv_key );
		if( CAstream )
			Release( CAstream );
		return rc;
	}
	return PS_SUCCESS;
}
#endif


ATEXIT( CloseSSL )
{
	if( ssl_global.flags.bInited ) {
	}
}


void CloseSession( PCLIENT pc )
{
	if( pc->ssl_session )
	{
		//matrixSslDeleteSessionId(pc->ssl_session->sid);

		Release( pc->ssl_session );
		pc->ssl_session = NULL;
	}
	if( !( pc->dwFlags & ( CF_CLOSED|CF_CLOSING ) ) )
		RemoveClient( pc );

}

static int logerr( const char *str, size_t len, void *userdata ) {
	lprintf( "%p: %s", userdata, str );
	return 0;
}



static int handshake( PCLIENT pc ) {
	struct ssl_session *ses = pc->ssl_session;
	if (!SSL_is_init_finished(ses->ssl)) {
		int r;
#ifdef DEBUG_SSL_IO
		lprintf( "doing handshake...." );
#endif
		/* NOT INITIALISED */

		r = SSL_do_handshake(ses->ssl);
#ifdef DEBUG_SSL_IO
		lprintf( "handle data posted to SSL? %d", r );
#endif
		if( r == 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
			r = SSL_get_error( ses->ssl, r );
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
#ifdef DEBUG_SSL_IO
			lprintf( "SSL_Read failed... %d", r );
#endif
			return -1;
		}
		if (r < 0) {

			r = SSL_get_error(ses->ssl, r);
			if( SSL_ERROR_SSL == r ) {
#ifdef DEBUG_SSL_IO
				lprintf( "SSL_Read failed... %d", r );
#endif
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
				return -1;
			}
			if (SSL_ERROR_WANT_READ == r) 
			{
				size_t pending = BIO_ctrl_pending( ses->wbio);
				if (pending > 0) {
					int read;
					if( pending > ses->obuflen ) {
						if( ses->obuffer )
							Deallocate( uint8_t *, ses->obuffer );
						ses->obuffer = NewArray( uint8_t, ses->obuflen = pending*2 );
					}
					read = BIO_read(ses->wbio, ses->obuffer, (int)pending);
#ifdef DEBUG_SSL_IO
					lprintf( "send %d for handshake", read );
#endif
					if (read > 0)
						SendTCP( pc, ses->obuffer, read );
				}
			}
			else {
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
			}
			return 0;
		}
		return 2;
	}
	else {
		/* SSL IS INITIALISED */
		return 1;
	}
}


static void ssl_ReadComplete( PCLIENT pc, POINTER buffer, size_t length )
{
#ifdef DEBUG_SSL_IO
	lprintf( "SSL Read complete %p %zd", buffer, length );
#endif
	if( pc->ssl_session )
	{
		if( buffer )
		{
			size_t len;
			int hs_rc;
			len = BIO_write( pc->ssl_session->rbio, buffer, (int)length );
#ifdef DEBUG_SSL_IO
			lprintf( "Wrote %zd", len );
#endif
			if( len < length ) {
				lprintf( "Protocol failure?" );
				Release( pc->ssl_session );
				RemoveClient( pc );
				return;
			}

			if( !( hs_rc = handshake( pc ) ) ) {
#ifdef DEBUG_SSL_IO
				// normal condition...
				lprintf( "Receive handshake not complete iBuffer" );
#endif
				ReadTCP( pc, pc->ssl_session->ibuffer, pc->ssl_session->ibuflen );
				return;
			}
			// == 1 if is already done, and not newly done
			if( hs_rc == 2 ) {
				// newly completed handshake.
				if( pc->ssl_session->dwOriginalFlags & CF_CPPREAD )
					pc->ssl_session->cpp_user_read( pc->psvRead, NULL, 0 );
				else
					pc->ssl_session->user_read( pc, NULL, 0 );
				len = 0;
			}
			else if( hs_rc == 1 )
			{
				int result;
				result = SSL_read( pc->ssl_session->ssl, pc->ssl_session->dbuffer, (int)pc->ssl_session->dbuflen );
#ifdef DEBUG_SSL_IO
				lprintf( "normal read - just get the data from the other buffer : %d", result );
#endif
				if( result == -1 ) {
					lprintf( "SSL_Read failed." );
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
					RemoveClient( pc );
					return;
				}
			}
			else
				len = 0;

			{
				// the read generated write data, output that data
				size_t pending = BIO_ctrl_pending( pc->ssl_session->wbio );
#ifdef DEBUG_SSL_IO
				lprintf( "pending to send is %zd", pending );
#endif
				if( pending > 0 ) {
					int read = BIO_read( pc->ssl_session->wbio, pc->ssl_session->obuffer, (int)pc->ssl_session->obuflen );
#ifdef DEBUG_SSL_IO
					lprintf( "Send pending %p %d", pc->ssl_session->obuffer, read );
#endif
					SendTCP( pc, pc->ssl_session->obuffer, read );
				}
			}
			// do was have any decrypted data to give to the application?
			if( len > 0 ) {
				if( pc->ssl_session->dwOriginalFlags & CF_CPPREAD )
					pc->ssl_session->cpp_user_read( pc->psvRead, pc->ssl_session->dbuffer, len );
				else
					pc->ssl_session->user_read( pc, pc->ssl_session->dbuffer, len );
			}
			else if( len == 0 ) {

			}
			else {
				lprintf( "SSL_Read failed." );
				ERR_print_errors_cb( logerr, (void*)__LINE__ );
				RemoveClient( pc );
			}

		}
		else {
			pc->ssl_session->ibuffer = NewArray( uint8_t, pc->ssl_session->ibuflen = 4096 );
			pc->ssl_session->dbuffer = NewArray( uint8_t, pc->ssl_session->dbuflen = 4096 );
			{
				int r;
				if( ( r = SSL_do_handshake( pc->ssl_session->ssl ) ) < 0 ) {
					//char buf[256];
					//r = SSL_get_error( ses->ssl, r );
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
					//lprintf( "err: %s", ERR_error_string( r, buf ) );
				}
				{
					// the read generated write data, output that data
					size_t pending = BIO_ctrl_pending( pc->ssl_session->wbio );
					if( pending > 0 ) {
						int read;
						if( pending > pc->ssl_session->obuflen ) {
							if( pc->ssl_session->obuffer )
								Deallocate( uint8_t *, pc->ssl_session->obuffer );
							pc->ssl_session->obuffer = NewArray( uint8_t, pc->ssl_session->obuflen = pending * 2 );
						}
						read = BIO_read( pc->ssl_session->wbio, pc->ssl_session->obuffer, (int)pc->ssl_session->obuflen );
						SendTCP( pc, pc->ssl_session->obuffer, read );
					}
				}
			}
		}
		if( pc->ssl_session )
			ReadTCP( pc, pc->ssl_session->ibuffer, pc->ssl_session->ibuflen );
	}
}

LOGICAL ssl_Send( PCLIENT pc, POINTER buffer, size_t length )
{
	int len;
	int32_t len_out;
	struct ssl_session *ses = pc->ssl_session;
	while( length ) {
		len = SSL_write( pc->ssl_session->ssl, buffer, (int)length );
		if (len < 0) {
			ERR_print_errors_cb(logerr, (void*)__LINE__);
			return FALSE;
		}
		length -= (size_t)len;

		// signed/unsigned comparison here.
		// the signed value is known to be greater than 0 and less than max unsigned int
		// so it is in a valid range to check, and is NOT a warning or error condition EVER.
		if( SUS_GT( len, int, ses->obuflen, size_t ) )
		{
			Release( ses->obuffer );
			ses->obuffer = NewArray( uint8_t, len * 2 );
			ses->obuflen = len * 2;
		}
		len_out = BIO_read( pc->ssl_session->wbio, ses->obuffer, (int)ses->obuflen );
		SendTCP( pc, ses->obuffer, len_out );
	}

	return TRUE;

}




static LOGICAL ssl_InitLibrary( void ){
	if( !ssl_global.flags.bInited )
	{
		SSL_library_init();
		//tls_init();
		//ssl_global.tls_config = tls_config_new();

		SSL_load_error_strings();
		ERR_load_BIO_strings();
		OpenSSL_add_all_algorithms();
		ssl_global.flags.bInited = 1;
	}
	return TRUE;
}

static void ssl_InitSession( struct ssl_session *ses ) {
	ses->rbio = BIO_new( BIO_s_mem() );
	ses->wbio = BIO_new( BIO_s_mem() );

	SSL_set_bio( ses->ssl, ses->rbio, ses->wbio );

}

static void ssl_CloseCallback( PCLIENT pc ) {
	struct ssl_session *ses = pc->ssl_session;
	if (!ses) {
		lprintf("already closed?");
		return;
	}
	pc->ssl_session = NULL;

	if( ses->dwOriginalFlags & CF_CPPCLOSE )
		ses->cpp_user_close( pc->psvClose );
	else
		ses->user_close( pc );

	Release( ses->dbuffer );
	Release( ses->ibuffer );
	Release( ses->obuffer );

	if( ses->cert ) {
		EVP_PKEY_free( ses->cert->pkey );
		X509_free( ses->cert->x509 );
		Release( ses->cert );
	}
	SSL_free( ses->ssl );
	SSL_CTX_free( ses->ctx );
	// these are closed... with the ssl connection.
	//BIO_free( ses->rbio );
	//BIO_free( ses->wbio );

	Release( ses );
}

static void ssl_ClientConnected( PCLIENT pcServer, PCLIENT pcNew ) {
	struct ssl_session *ses;
	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );

	ses->ssl = SSL_new( pcServer->ssl_session->ctx );

	ssl_InitSession( ses );

	SSL_set_accept_state( ses->ssl );

	pcNew->ssl_session = ses;

	if( pcServer->ssl_session->dwOriginalFlags & CF_CPPCONNECT )
		pcServer->ssl_session->cpp_user_connected( pcServer->psvConnect, pcNew );
	else
		pcServer->ssl_session->user_connected( pcServer, pcNew);


	ses->user_read = pcNew->read.ReadComplete;
	ses->cpp_user_read = pcNew->read.CPPReadComplete;

	ses->user_close = pcNew->close.CloseCallback;
	ses->cpp_user_close = pcNew->close.CPPCloseCallback;

	pcNew->read.ReadComplete = ssl_ReadComplete;
	pcNew->dwFlags &= ~CF_CPPREAD;
	pcNew->close.CloseCallback = ssl_CloseCallback;
	pcNew->dwFlags &= ~CF_CPPCLOSE;


	ses->dwOriginalFlags = pcServer->ssl_session->dwOriginalFlags;

}

struct info_params {
	char *password;
	int passlen;
};


static int pem_password( char *buf, int size, int rwflag, void *u )
{
	// rwflag = 0  = used for decryption
	// rwflag = 1  = used for encryption
	struct info_params *params = (struct info_params*)u;
	int len;
	memcpy( buf, params->password, len = (size<params->passlen ? size : params->passlen) );

	return len;
}

LOGICAL ssl_BeginServer( PCLIENT pc, POINTER cert, size_t certlen, POINTER keypair, size_t keylen, POINTER keypass, size_t keypasslen ) {
	struct ssl_session * ses;
	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );
	ssl_InitLibrary();
	pc->flags.bSecure = 1;

	if( !cert ) {
		ses->cert = MakeRequest();
	} else {
		struct internalCert *certStruc = New( struct internalCert );
		BIO *keybuf = BIO_new( BIO_s_mem() );
		X509 *x509, *result;
		certStruc->chain = NULL;
		certStruc->chain = sk_X509_new_null();
		//PKCS12_parse( )
		BIO_write( keybuf, cert, (int)certlen );
		do {
			x509 = X509_new();
			result = PEM_read_bio_X509( keybuf, &x509, NULL, NULL );
			if( result )
				sk_X509_push( certStruc->chain, x509 );
			else
				X509_free( x509 );
		} while( result );
		BIO_free( keybuf );
		ses->cert = certStruc;
	}

	if( !keypair ) {
		if( !ses->cert->pkey )
			ses->cert->pkey = genKey();
	} else {
		BIO *keybuf = BIO_new( BIO_s_mem() );
		struct info_params params;
		ses->cert->pkey = EVP_PKEY_new();
		BIO_write( keybuf, keypair, (int)keylen );
		params.password = (char*)keypass;
		params.passlen = (int)keypasslen;
		PEM_read_bio_PrivateKey( keybuf, &ses->cert->pkey, pem_password, &params );
		BIO_free( keybuf );
	}

	ses->ctx = SSL_CTX_new( TLSv1_2_server_method() );
	{
		int r;
		SSL_CTX_set_cipher_list( ses->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" );

		r = SSL_CTX_set0_chain( ses->ctx, ses->cert->chain );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		r = SSL_CTX_use_certificate( ses->ctx, sk_X509_value( ses->cert->chain, 0 ) );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		r = SSL_CTX_use_PrivateKey( ses->ctx, ses->cert->pkey );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
		{
			int n;
			for( n = 1; n < sk_X509_num( ses->cert->chain ); n++ ) {
				r = SSL_CTX_add_extra_chain_cert( ses->ctx, sk_X509_value( ses->cert->chain, n ) );
				if( r <= 0 ) {
					ERR_print_errors_cb( logerr, (void*)__LINE__ );
				}
			}
		}
		r = SSL_CTX_check_private_key( ses->ctx );
		if( r <= 0 ) {
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
		}
	}

	ses->dwOriginalFlags = pc->dwFlags;

	ses->user_connected = pc->connect.ClientConnected;
	ses->cpp_user_connected = pc->connect.CPPClientConnected;
	pc->connect.ClientConnected = ssl_ClientConnected;
	pc->dwFlags &= ~CF_CPPCONNECT;

	pc->ssl_session = ses;
	// at this point pretty much have to assume
	// that it will be OK.
	return TRUE;

}


LOGICAL ssl_GetPrivateKey( PCLIENT pc, POINTER *keyoutbuf, size_t *keylen ) {
	if( pc && keyoutbuf && keylen ) {
		struct ssl_session * ses = pc->ssl_session;
		BIO *keybuf = BIO_new( BIO_s_mem() );
		if( !ses->cert->pkey )
			ses->cert->pkey = genKey();
		PEM_write_bio_PrivateKey( keybuf, ses->cert->pkey, NULL, NULL, 0, NULL, NULL );
		(*keylen) = BIO_pending( keybuf );
		(*keyoutbuf) = NewArray( uint8_t, (*keylen) + 1 );
		BIO_read( keybuf, (*keyoutbuf), (int)(*keylen) );
		((char*)*keyoutbuf)[(*keylen)] = 0;
		return TRUE;
	}
	return FALSE;
}

//typedef long (*BIO_callback_fn)(BIO *b, int oper, const char *argp, int argi,
//                                 long argl, long ret);
//
// void BIO_set_callback(BIO *b, BIO_callack_fn cb);



LOGICAL ssl_BeginClientSession( PCLIENT pc, POINTER client_keypair, size_t client_keypairlen, POINTER keypass, size_t keypasslen, POINTER rootCert, size_t rootCertLen )
{
	struct ssl_session * ses;
	ssl_InitLibrary();

	ses = New( struct ssl_session );
	MemSet( ses, 0, sizeof( struct ssl_session ) );
	{
		ses->ctx = SSL_CTX_new( TLSv1_2_client_method() );
		SSL_CTX_set_cipher_list( ses->ctx, "ALL:!ADH:!LOW:!EXP:!MD5:@STRENGTH" );
		ses->cert = New( struct internalCert );
		if( !client_keypair )
			ses->cert->pkey = genKey();
		else {
			BIO *keybuf = BIO_new( BIO_s_mem() );
			struct info_params params;
			BIO_write( keybuf, client_keypair, (int)client_keypairlen );
			params.password = (char*)keypass;
			params.passlen = (int)keypasslen;
			PEM_read_bio_PrivateKey( keybuf, &ses->cert->pkey, pem_password, &params );
			BIO_free( keybuf );
		}
		SSL_CTX_use_PrivateKey( ses->ctx, ses->cert->pkey );
	}
	ses->ssl = SSL_new( ses->ctx );
	if( rootCert ) {
		BIO *keybuf = BIO_new( BIO_s_mem() );
		X509 *cert;
		cert = X509_new();
		BIO_write( keybuf, rootCert, (int)rootCertLen );
		PEM_read_bio_X509( keybuf, &cert, NULL, NULL );
		BIO_free( keybuf );
		SSL_CTX_add_extra_chain_cert( ses->ctx, cert );
	}
	ssl_InitSession( ses );

	SSL_set_connect_state( ses->ssl );

	ses->dwOriginalFlags = pc->dwFlags;
	ses->user_read = pc->read.ReadComplete;
	ses->cpp_user_read = pc->read.CPPReadComplete;
	pc->read.ReadComplete = ssl_ReadComplete;
	pc->dwFlags &= ~CF_CPPREAD;

	//SSL_use_certificate( )

	pc->ssl_session = ses;

	//ssl_accept( pc->ssl_session->ssl );
	lprintf( "Warning: This is meant to be called during connect; no longer internally auto handshakes here; but should upon returning from here instead." );
/*
	ssl_ReadComplete( pc, NULL, 0 );

	{
		int r;
		if( ( r = SSL_do_handshake( ses->ssl ) ) < 0 ) {
			//char buf[256];
			//r = SSL_get_error( ses->ssl, r );
			ERR_print_errors_cb( logerr, (void*)__LINE__ );
			//lprintf( "err: %s", ERR_error_string( r, buf ) );
		}
		{
			// the read generated write data, output that data
			size_t pending = BIO_ctrl_pending( pc->ssl_session->wbio );
			if( pending > 0 ) {
				int read;
				if( pending > ses->obuflen ) {
					if( ses->obuffer )
						Deallocate( uint8_t *, ses->obuffer );
					ses->obuffer = NewArray( uint8_t, ses->obuflen = pending * 2 );
				}
				read = BIO_read( pc->ssl_session->wbio, pc->ssl_session->obuffer, (int)pc->ssl_session->obuflen );
				SendTCP( pc, pc->ssl_session->obuffer, read );
			}
		}
	}
*/

	return TRUE;
}



//--------------------- Make Cert Request
//#include <stdio.h>
//#include <stdlib.h>
//#include <openssl/x509.h>
//#include <openssl/x509v3.h>
//#include <openssl/err.h>
//#include <openssl/pem.h>
//#include <openssl/evp.h>

// Fatal error; abort with message, including file and line number 
//
static int cb(const char *str, size_t len, void *u)  {
	lprintf( "%s", str);
	return 1;

}

static void fatal_error(const char *file, int line, const char *msg)
{ 
	fprintf(stderr, "**FATAL** %s:%i %s\n", file, line, msg);
	fprintf( stderr, "specific error available, but not dumped; ERR_print_errors_fp is gone" );
	ERR_print_errors_cb( cb, 0 );
	//ERR_print_errors_fp(stderr);
	exit(-1); 
} 

#define fatal(msg) fatal_error(__FILE__, __LINE__, msg) 

// Parameter settings for this cert 
// 
#define ENTRIES (sizeof(entries)/sizeof(entries[0]))


// declare array of entries to assign to cert 
struct entry 
{ 
	const char *key;
	const char *value;
}; 

struct entry entries[] =
{ 
    { "countryName", "US" }, 
    { "stateOrProvinceName", "NV" },
    { "localityName", "Las Vegas" },
    { "organizationName", "d3x0r.org" },
    { "organizationalUnitName", "Development" }, 
    { "commonName", "Internal Project" }, 
}; 

// main --- 
// 
// 
//int main(int argc, char *argv[])
struct internalCert * MakeRequest( void )
{ 
	 struct internalCert *cert = NewArray( struct internalCert, 1 );
	 int ca_len;
	 int key_len; // already cached key
	 char* ca;
	 char* key;

	 BIO *keybuf = BIO_new( BIO_s_mem() );
	 MemSet( cert, 0, sizeof( struct internalCert ) );
	 // Create evp obj to hold our rsakey 
	 // 
	 if( !(cert->pkey = EVP_PKEY_new()) )
		 fatal( "Could not create EVP object" );


#ifndef __NO_OPTIONS__
	 ca_len = SACK_GetProfileInt( "TLS", "CA Length", 0 );
#else
	 ca_len = 0;
#endif
	 if( ca_len ) {
		 ca = NewArray( char, ca_len + 2 );

#ifndef __NO_OPTIONS__
		 SACK_GetProfileString( "TLS", "CA Cert", "", ca, ca_len + 1 );
#endif
		 BIO_write( keybuf, ca, ca_len );
		 Deallocate( void *, ca );

		 cert->x509 = X509_new();
		 PEM_read_bio_X509( keybuf, &cert->x509, NULL, NULL );
	 }



#ifndef __NO_OPTIONS__
	 key_len = SACK_GetProfileInt( "TLS", "Key Length", 0 );
#else
	 key_len = 0;
#endif
	if( key_len ) {
		 key = NewArray( char, key_len + 2 );
#ifndef __NO_OPTIONS__
		 SACK_GetProfileString( "TLS", "Key", "", (TEXTCHAR*)key, key_len + 1 );
#endif
		 BIO_write( keybuf, key, key_len );
		 Deallocate( void *, key );
		 PEM_read_bio_PrivateKey( keybuf, &cert->pkey, NULL, NULL );
	} else {
		RSA *rsa = RSA_new();
		BIGNUM *bne = BN_new();
		int ret;
		ret = BN_set_word( bne, kExp );
		if( ret != 1 ) {
			BN_free( bne );
			RSA_free( rsa );
			fatal( "Could not set Bignum?" );
			return NULL;
		}
		RSA_generate_key_ex( rsa, serverKBits, bne, NULL );
		if( !(EVP_PKEY_set1_RSA( cert->pkey, rsa )) )
			fatal( "Could not assign RSA key to EVP object" );

		BN_free( bne );
		RSA_free( rsa );
		{
			PEM_write_bio_PrivateKey( keybuf, cert->pkey,               /* our key from earlier */
				NULL, //EVP_des_ede3_cbc(), /* default cipher for encrypting the key on disk */
				NULL, //"replace_me",       /* passphrase required for decrypting the key on disk */
				10,                 /* length of the passphrase string */
				NULL,               /* callback for requesting a password */
				NULL                /* data to pass to the callback */
			);

			ca_len = BIO_pending( keybuf );
			ca = NewArray( char, ca_len + 1 );
			BIO_read( keybuf, ca, ca_len );
			ca[ca_len] = 0;
			SACK_WriteProfileInt( "TLS", "Key Length", ca_len );
			SACK_WriteProfileString( "TLS", "Key", (TEXTCHAR*)ca );
			Release( ca );
		}
	}

    // seed openssl's prng 
    // 
    /* 
    if (RAND_load_file("/dev/random", -1)) 
        fatal("Could not seed prng"); 
    */ 
    // Generate the RSA key; we don't assign a callback to monitor progress 
    // since generating keys is fast enough these days 
    // 

	if (!cert->x509)
	{
		X509 * x509;
		if (!cert->x509)
			cert->x509 = X509_new();
		x509 = cert->x509;
		ASN1_INTEGER_set( X509_get_serialNumber( x509 ), 1 );
		X509_gmtime_adj( X509_get_notBefore( x509 ), 0 );
		// (60 seconds * 60 minutes * 24 hours * 365 days) = 31536000.
		X509_gmtime_adj( X509_get_notAfter( x509 ), 31536000L );
		X509_set_pubkey( x509, cert->pkey );
		{
			X509_NAME *name;
			TEXTCHAR commonName[48];
			TEXTCHAR org[48];
			TEXTCHAR country[8];
#ifndef __NO_OPTIONS__
			SACK_GetProfileString( "TLS", "Default CA Common Name", "d3x0r.org", commonName, 48 );
			SACK_GetProfileString( "TLS", "Default CA Org", "Freedom Collective", org, 48 );
			SACK_GetProfileString( "TLS", "Default CA Country", "US", country, 8 );
#else
			strcpy( commonName, "d3x0r.org" );
			strcpy( org, "Freedom Collective" );
			strcpy( country, "US" );
#endif
			name = X509_get_subject_name( x509 );
			X509_NAME_add_entry_by_txt( name, "C", MBSTRING_ASC,
				(unsigned char *)country, -1, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "O", MBSTRING_ASC,
				(unsigned char *)org, -1, -1, 0 );
			X509_NAME_add_entry_by_txt( name, "CN", MBSTRING_ASC,
				(unsigned char *)commonName, -1, -1, 0 );

			X509_set_issuer_name( x509, name );

			X509_sign( x509, cert->pkey, EVP_sha512() );

			{
				PEM_write_bio_X509( keybuf, x509 );
				ca_len = BIO_pending( keybuf );
				ca = NewArray( char, ca_len + 1 );

				BIO_read( keybuf, ca, ca_len );
				ca[ca_len] = 0;
				SACK_WriteProfileInt( "TLS", "CA Length", ca_len );
				SACK_WriteProfileString( "TLS", "CA Cert", (TEXTCHAR*)ca );
				Release( ca );
			}

		}
	}

#if 0
    // create request object 
    // 
    if (!(cert->req = X509_REQ_new())) 
        fatal("Failed to create X509_REQ object"); 
    X509_REQ_set_pubkey(cert->req, cert->pkey_ca); 

    // create and fill in subject object 
    // 
    if (!(subj = X509_NAME_new())) 
        fatal("Failed to create X509_NAME object"); 

    for (i = 0; i < ENTRIES; i++) 
    { 
        int nid;                  // ASN numeric identifier 
        X509_NAME_ENTRY *ent; 

        if ((nid = OBJ_txt2nid(entries[i].key)) == NID_undef) 
        { 
            fprintf(stderr, "Error finding NID for %s\n", entries[i].key); 
            fatal("Error on lookup"); 
        } 
        if (!(ent = X509_NAME_ENTRY_create_by_NID(NULL, nid, MBSTRING_ASC, 
            (unsigned char*)entries[i].value, - 1))) 
            fatal("Error creating Name entry from NID"); 

        if (X509_NAME_add_entry(subj, ent, -1, 0) != 1) 
            fatal("Error adding entry to Name"); 
    } 
    if (X509_REQ_set_subject_name(cert->req, subj) != 1) 
        fatal("Error adding subject to request"); 

    // request is filled in and contains our generated public key; 
    // now sign it 
    // 

    if (!(X509_REQ_sign(cert->req, cert->pkey, EVP_sha1() )))
        fatal("Error signing request"); 


	/*
    // write output files 
    // 
    if (!(fp = fopen(REQ_FILE, "w"))) 
        fatal("Error writing to request file"); 
    if (PEM_write_X509_REQ(fp, cert->req) != 1) 
        fatal("Error while writing request"); 
    fclose(fp); 

    if (!(fp = fopen(KEY_FILE, "w"))) 
        fatal("Error writing to private key file"); 
    if (PEM_write_PrivateKey(fp, cert->pkey, NULL, NULL, 0, 0, NULL) != 1) 
        fatal("Error while writing private key"); 
    fclose(fp); 
	*/
#endif
	 return cert;
} 




SACK_NETWORK_NAMESPACE_END
#endif
#endif
