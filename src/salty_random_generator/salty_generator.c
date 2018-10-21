#include <stdhdrs.h>
#include <sha1.h>
#ifdef SACK_BAG_EXPORTS
#define SHA2_SOURCE
#endif
#include <sha2.h>
#include "../contrib/sha3lib/sha3.h"

#ifndef SALTY_RANDOM_GENERATOR_SOURCE
#define SALTY_RANDOM_GENERATOR_SOURCE
#endif
#include <salty_generator.h>


#define MY_MASK_MASK(n,length)	(MASK_TOP_MASK(length) << ((n)&0x7) )
#define MY_GET_MASK(v,n,mask_size)  ( ( ((MASKSET_READTYPE*)((((uintptr_t)v))+(n)/CHAR_BIT))[0]											\
 & MY_MASK_MASK(n,mask_size) )																									\
	>> (((n))&0x7))


struct random_context {
	LOGICAL use_version2 : 1;
	LOGICAL use_version2_256 : 1;
	LOGICAL use_version3 : 1;

	SHA1Context sha1_ctx;
	sha512_ctx  sha512;
	sha256_ctx  sha256;
	sha3_ctx_t  sha3;

	POINTER salt;
	size_t salt_size;
	void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size );
	uintptr_t psv_user;
	uint8_t entropy[SHA1HashSize];
	uint8_t entropy2[SHA512_DIGEST_SIZE];
	uint8_t entropy2_256[SHA256_DIGEST_SIZE];
#define SHA3_DIGEST_SIZE 64
	uint8_t entropy3[SHA3_DIGEST_SIZE];
	size_t bits_used;
	size_t bits_avail;
};

static void NeedBits( struct random_context *ctx )
{
	if( ctx->getsalt )
		ctx->getsalt( ctx->psv_user, &ctx->salt, &ctx->salt_size );
	else
		ctx->salt_size = 0;
	if( ctx->use_version3 ) {
		if( ctx->salt_size )
			sha3_update( &ctx->sha3, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
		sha3_final( &ctx->sha3, ctx->entropy3 );
		sha3_init( &ctx->sha3, SHA3_DIGEST_SIZE );
		sha3_update( &ctx->sha3, ctx->entropy3, SHA3_DIGEST_SIZE );
		ctx->bits_avail = sizeof( ctx->entropy3 ) * 8;
	} else if( ctx->use_version2_256 ) {
		if( ctx->salt_size )
			sha256_update( &ctx->sha256, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
		sha256_final( &ctx->sha256, ctx->entropy2_256 );
		sha256_init( &ctx->sha256 );
		sha256_update( &ctx->sha256, ctx->entropy2_256, SHA256_DIGEST_SIZE );
		ctx->bits_avail = sizeof( ctx->entropy2_256 ) * 8;
	} else if( ctx->use_version2 )
	{
		if( ctx->salt_size )
			sha512_update( &ctx->sha512, (const uint8_t*)ctx->salt, (unsigned int)ctx->salt_size );
		sha512_final( &ctx->sha512, ctx->entropy2 );
		sha512_init( &ctx->sha512 );
		sha512_update( &ctx->sha512, ctx->entropy2, SHA512_DIGEST_SIZE );
		ctx->bits_avail = sizeof( ctx->entropy2 ) * 8;
	}
	else
	{
		if( ctx->salt_size )
			SHA1Input( &ctx->sha1_ctx, (const uint8_t*)ctx->salt, ctx->salt_size );
		SHA1Result( &ctx->sha1_ctx, ctx->entropy );
		SHA1Reset( &ctx->sha1_ctx );
		SHA1Input( &ctx->sha1_ctx, ctx->entropy, SHA1HashSize );
		ctx->bits_avail = sizeof( ctx->entropy ) * 8;
	}
	ctx->bits_used = 0;
}

struct random_context *SRG_CreateEntropyInternal( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user
                                                , LOGICAL version2 
                                                , LOGICAL version2_256
                                                , LOGICAL version3
                                                )
{
	struct random_context *ctx = New( struct random_context );
	ctx->use_version3 = version3;
	ctx->use_version2_256 = version2_256;
	ctx->use_version2 = version2;
	if( ctx->use_version3 )
		sha3_init( &ctx->sha3, SHA3_DIGEST_SIZE );
	else if( ctx->use_version2_256 )
		sha256_init( &ctx->sha256 );
	else if( ctx->use_version2 )
		sha512_init( &ctx->sha512 );
	else
		SHA1Reset( &ctx->sha1_ctx );
	ctx->getsalt = getsalt;
	ctx->psv_user = psv_user;
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
	return ctx;
}

struct random_context *SRG_CreateEntropy( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, FALSE, FALSE );
}

struct random_context *SRG_CreateEntropy2( void (*getsalt)( uintptr_t, POINTER *salt, size_t *salt_size ), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, TRUE, FALSE, FALSE );
}

struct random_context *SRG_CreateEntropy2_256( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, TRUE, FALSE );
}

struct random_context *SRG_CreateEntropy3( void( *getsalt )(uintptr_t, POINTER *salt, size_t *salt_size), uintptr_t psv_user )
{
	return SRG_CreateEntropyInternal( getsalt, psv_user, FALSE, FALSE, TRUE );
}

void SRG_DestroyEntropy( struct random_context **ppEntropy )
{
	Release( (*ppEntropy) );
	(*ppEntropy) = NULL;
}

void SRG_GetEntropyBuffer( struct random_context *ctx, uint32_t *buffer, uint32_t bits )
{
	uint32_t tmp;
	uint32_t partial_tmp;
	uint32_t partial_bits = 0;
	uint32_t get_bits;
	uint32_t resultBits = 0;
	if( !ctx ) DebugBreak();
	//if( ctx->bits_used > 512 ) DebugBreak();
	do {
		if( bits > sizeof( tmp ) * 8 )
			get_bits = sizeof( tmp ) * 8;
		else
			get_bits = bits;

		// if there were 1-31 bits of data in partial, then can only get 32-partial max.
		if( 32 < (get_bits + partial_bits) )
			get_bits = 32 - partial_bits;
		// check1 :
		//    if get_bits == 32
		//    but bits_used is 1-7, then it would have to pull 5 bytes to get the 32 required
		//    so truncate get_bits to 25-31 bits
		if( 32 < (get_bits + (ctx->bits_used & 0x7)) )
			get_bits = (32 - (ctx->bits_used & 0x7));
		// if resultBits is 1-7 offset, then would have to store up to 5 bytes of value
		//    so have to truncate to just the up to 4 bytes that will fit.
		if( (get_bits+ resultBits) > 32 )
			get_bits = 32 - resultBits;
		// only greater... if equal just grab the bits.
		if( (get_bits + ctx->bits_used) > ctx->bits_avail ) {
			// if there are any bits left, grab the partial bits.
			if( ctx->bits_avail > ctx->bits_used ) {
				partial_bits = (uint32_t)(ctx->bits_avail - ctx->bits_used);
				if( partial_bits > get_bits ) partial_bits = get_bits;
				// partial can never be greater than 32; input is only max of 32
				//if( partial_bits > (sizeof( partial_tmp ) * 8) )
				//	partial_bits = (sizeof( partial_tmp ) * 8);
				if( ctx->use_version3 )
					partial_tmp = MY_GET_MASK( ctx->entropy3, ctx->bits_used, partial_bits );
				else if( ctx->use_version2_256 )
					partial_tmp = MY_GET_MASK( ctx->entropy2_256, ctx->bits_used, partial_bits );
				else if( ctx->use_version2 )
					partial_tmp = MY_GET_MASK( ctx->entropy2, ctx->bits_used, partial_bits );
				else
					partial_tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, partial_bits );
			}
			NeedBits( ctx );
			bits -= partial_bits;
		}
		else {
			if( ctx->use_version3 )
				tmp = MY_GET_MASK( ctx->entropy3, ctx->bits_used, get_bits );
			else if( ctx->use_version2_256 )
				tmp = MY_GET_MASK( ctx->entropy2_256, ctx->bits_used, get_bits );
			else if( ctx->use_version2 )
				tmp = MY_GET_MASK( ctx->entropy2, ctx->bits_used, get_bits );
			else
				tmp = MY_GET_MASK( ctx->entropy, ctx->bits_used, get_bits );
			ctx->bits_used += get_bits;
			//if( ctx->bits_used > 512 ) DebugBreak();
			if( partial_bits ) {
				tmp = partial_tmp | (tmp << partial_bits);
				partial_bits = 0;
			}
			(*buffer) = tmp << resultBits;
			resultBits += get_bits;
			while( resultBits >= 8 ) {
#if defined( __cplusplus ) || defined( __GNUC__ )
				buffer = (uint32_t*)(((uintptr_t)buffer) + 1);
#else
				((intptr_t)buffer)++;
#endif
				resultBits -= 8;
			}
			//if( get_bits > bits ) DebugBreak();
			bits -= get_bits;
		}
	} while( bits );
}

int32_t SRG_GetEntropy( struct random_context *ctx, int bits, int get_signed )
{
	int32_t result;
	SRG_GetEntropyBuffer( ctx, (uint32_t*)&result, bits );
	if( get_signed )
		if( result & ( 1 << ( bits - 1 ) ) )
		{
			uint32_t negone = ~0;
			negone <<= bits;
			return (int32_t)( result | negone );
		}
	return result;
}

void SRG_ResetEntropy( struct random_context *ctx )
{
	if( ctx->use_version3 )
		sha3_init( &ctx->sha3, SHA3_DIGEST_SIZE );
	else if( ctx->use_version2_256 )
		sha256_init( &ctx->sha256 );
	else if( ctx->use_version2 )
		sha512_init( &ctx->sha512 );
	else
		SHA1Reset( &ctx->sha1_ctx );
	ctx->bits_used = 0;
	ctx->bits_avail = 0;
}

void SRG_FeedEntropy( struct random_context *ctx, const uint8_t *salt, size_t salt_size )
{
	if( ctx->use_version3 )
		sha3_update( &ctx->sha3, salt, (unsigned int)salt_size );
	else if( ctx->use_version2_256 )
		sha256_update( &ctx->sha256, salt, (unsigned int)salt_size );
	else if( ctx->use_version2 )
		sha512_update( &ctx->sha512, salt, (unsigned int)salt_size );
	else
		SHA1Input( &ctx->sha1_ctx, salt, salt_size );
}

void SRG_SaveState( struct random_context *ctx, POINTER *external_buffer_holder )
{
	if( !(*external_buffer_holder) )
		(*external_buffer_holder) = New( struct random_context );
	MemCpy( (*external_buffer_holder), ctx, sizeof( struct random_context ) );
}

void SRG_RestoreState( struct random_context *ctx, POINTER external_buffer_holder )
{
	MemCpy( ctx, (external_buffer_holder), sizeof( struct random_context ) );
}


static void salt_generator(uintptr_t psv, POINTER *salt, size_t *salt_size ) {
	static uint32_t tick;
   (void)psv;
	tick = GetTickCount();
	salt[0] = &tick;
	salt_size[0] = sizeof( tick );
}

char *SRG_ID_Generator( void ) {
	static struct random_context *ctx;
	uint32_t buf[2*(16+16)];
	size_t outlen;
	if( !ctx ) ctx = SRG_CreateEntropy2( salt_generator, 0 );
	SRG_GetEntropyBuffer( ctx, buf, 8*(16+16) );
	return EncodeBase64Ex( (uint8*)buf, (16+16), &outlen, (const char *)1 );
}

char *SRG_ID_Generator_256( void ) {
	static struct random_context *_ctx[32];
	static uint32_t used[32];
	uint32_t buf[2 * (16 + 16)];
	size_t outlen;
	int usingCtx;
	static struct random_context *ctx;
	usingCtx = 0;
	do {
		while( used[++usingCtx] ) { if( ++usingCtx >= 32 ) usingCtx = 0; }
	} while( LockedExchange( used + usingCtx, 1 ) );
	ctx = _ctx[usingCtx];
	if( !ctx ) ctx = _ctx[usingCtx] = SRG_CreateEntropy2_256( salt_generator, 0 );
	SRG_GetEntropyBuffer( ctx, buf, 8 * (16 + 16) );
	used[usingCtx] = 0;
	return EncodeBase64Ex( (uint8*)buf, (16 + 16), &outlen, (const char *)1 );
}

char *SRG_ID_Generator3( void ) {
	static struct random_context *ctx;
	uint32_t buf[2 * (16 + 16)];
	size_t outlen;
	if( !ctx ) ctx = SRG_CreateEntropy3( salt_generator, 0 );
	SRG_GetEntropyBuffer( ctx, buf, 8 * (16 + 16) );
	return EncodeBase64Ex( (uint8*)buf, (16 + 16), &outlen, (const char *)1 );
}


#ifdef WIN32
#if 0  
// if standalone?
BOOL WINAPI DllMain(
	HINSTANCE hinstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved
						 )
{
	return TRUE;
}
#endif
// this is the watcom deadstart entry point.
// by supplying this routine, then the native runtime doesn't get pulled
// and no external clbr symbols are required.
//void __DLLstart( void )
//{
//}
#endif
