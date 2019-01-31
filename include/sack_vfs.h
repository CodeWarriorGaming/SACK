
#ifndef SACK_VFS_DEFINED
/* Header multiple inclusion protection symbol. */
#define SACK_VFS_DEFINED

#ifdef SACK_VFS_STATIC
#  ifdef SACK_VFS_SOURCE
#    define SACK_VFS_PROC
#  else
#    define SACK_VFS_PROC extern
#  endif
#else
#  ifdef SACK_VFS_SOURCE
#    define SACK_VFS_PROC EXPORT_METHOD
#  else
#    define SACK_VFS_PROC IMPORT_METHOD
#  endif
#endif

#ifdef __cplusplus
/* defined the file system partial namespace (under
   SACK_NAMESPACE probably)                         */
#define _SACK_VFS_NAMESPACE  namespace SACK_VFS {
/* Define the ending symbol for file system namespace. */
#define _SACK_VFS_NAMESPACE_END }
#else
#define _SACK_VFS_NAMESPACE 
#define _SACK_VFS_NAMESPACE_END
#endif
/* define the file system namespace end. */
#define SACK_VFS_NAMESPACE_END _SACK_VFS_NAMESPACE_END SACK_NAMESPACE_END 
/* define the file system namespace. */
#define SACK_VFS_NAMESPACE SACK_NAMESPACE _SACK_VFS_NAMESPACE 

SACK_VFS_NAMESPACE

// if the option to auto mount a file system is used, this is the
// name of the 'file system interface'  ( sack_get_filesystem_interface( SACK_VFS_FILESYSTEM_NAME ) )
#define SACK_VFS_FILESYSTEM_NAME WIDE("sack_shmem")

// open a volume at the specified pathname.
// if the volume does not exist, will create it.
// if the volume does exist, a quick validity check is made on it, and then the result is opened
// returns NULL if failure.  (permission denied to the file, or invalid filename passed, could be out of space... )
// same as load_cyrypt_volume with userkey and devkey NULL.
SACK_VFS_PROC struct volume * CPROC sack_vfs_load_volume( CTEXTSTR filepath );

// open a volume at the specified pathname.  Use the specified keys to encrypt it.
// if the volume does not exist, will create it.
// if the volume does exist, a quick validity check is made on it, and then the result is opened
// returns NULL if failure.  (permission denied to the file, or invalid filename passed, could be out of space... )
// if the keys are NULL same as load_volume.
SACK_VFS_PROC struct volume * CPROC sack_vfs_load_crypt_volume( CTEXTSTR filepath, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey );
// pass some memory and a memory length of the memory to use as a volume.
// if userkey and/or devkey are not NULL the memory is assume to be encrypted with those keys.
// the space is opened as readonly; write accesses/expanding operations will fail.
SACK_VFS_PROC struct volume * CPROC sack_vfs_use_crypt_volume( POINTER filemem, size_t size, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey );
// close a volume; release all resources; any open files will keep the volume open.
// when the final file closes the volume will complete closing.
SACK_VFS_PROC void            CPROC sack_vfs_unload_volume( struct volume * vol );
// remove unused extra allocated space at end of volume.  During working process, extra space is preallocated for
// things to be stored in.
SACK_VFS_PROC void            CPROC sack_vfs_shrink_volume( struct volume * vol );
// remove encryption from volume.
SACK_VFS_PROC LOGICAL         CPROC sack_vfs_decrypt_volume( struct volume *vol );
// change the key applied to a volume.
SACK_VFS_PROC LOGICAL         CPROC sack_vfs_encrypt_volume( struct volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 );
// create a signature of current directory of volume.
// can be used to validate content.  Returns 256 character hex string.
SACK_VFS_PROC const char *    CPROC sack_vfs_get_signature( struct volume *vol );
// pass an offset from memory start and the memory start...
// computes the distance, uses that to generate a signature
// returns BLOCK_SIZE length signature; recommend using at least 128 bits of it.
SACK_VFS_PROC const uint8_t * CPROC sack_vfs_get_signature2( POINTER disk, POINTER diskReal );

// ---------- Operations on files in volumes ------------------

// open a file, creates if does not exist.
SACK_VFS_PROC struct sack_vfs_file * CPROC sack_vfs_openfile( struct volume *vol, CTEXTSTR filename );
// check if a file exists (if it does not exist, and you don't want it created, can use this and not openfile)
SACK_VFS_PROC int CPROC sack_vfs_exists( struct volume *vol, const char * file );
// close a file.
SACK_VFS_PROC int CPROC sack_vfs_close( struct sack_vfs_file *file );
// get the current File Position Index (FPI).
SACK_VFS_PROC size_t CPROC sack_vfs_tell( struct sack_vfs_file *file );
// get the length of the file
SACK_VFS_PROC size_t CPROC sack_vfs_size( struct sack_vfs_file *file );
// set the current File Position Index (FPI).
SACK_VFS_PROC size_t CPROC sack_vfs_seek( struct sack_vfs_file *file, size_t pos, int whence );
// write starting at the current FPI.
SACK_VFS_PROC size_t CPROC sack_vfs_write( struct sack_vfs_file *file, const char * data, size_t length );
// read starting at the current FPI.
SACK_VFS_PROC size_t CPROC sack_vfs_read( struct sack_vfs_file *file, char * data, size_t length );
// sets the file length to the current FPI.
SACK_VFS_PROC size_t CPROC sack_vfs_truncate( struct sack_vfs_file *file );
// psv should be struct volume *vol;
// delete a filename.  Clear the space it was occupying.
SACK_VFS_PROC int CPROC sack_vfs_unlink_file( struct volume *vol, const char * filename );
// rename a file within the filesystem; if the target name exists, it is deleted.  If the target file is also open, it will be prevented from deletion; and duplicate filenames will end up exising(?)
SACK_VFS_PROC LOGICAL CPROC sack_vfs_rename( uintptr_t psvInstance, const char *original, const char *newname );
// -----------  directory interface commands. ----------------------

// returns find_info which is then used in subsequent commands.
SACK_VFS_PROC struct find_info * CPROC sack_vfs_find_create_cursor(uintptr_t psvInst,const char *base,const char *mask );
// reset find_info to the first directory entry.  returns 0 if no entry.
SACK_VFS_PROC int CPROC sack_vfs_find_first( struct find_info *info );
// closes a find cursor; returns 0.
SACK_VFS_PROC int CPROC sack_vfs_find_close( struct find_info *info );
// move to the next entry returns 0 if no entry.
SACK_VFS_PROC int CPROC sack_vfs_find_next( struct find_info *info );
// get file information for the file at the current cursor position...
SACK_VFS_PROC char * CPROC sack_vfs_find_get_name( struct find_info *info );
// get file information for the file at the current cursor position...
SACK_VFS_PROC size_t CPROC sack_vfs_find_get_size( struct find_info *info );

#ifdef __cplusplus
namespace fs {
#endif
	struct volume;
	struct sack_vfs_file;
	struct find_info;
	// open a volume at the specified pathname.
	// if the volume does not exist, will create it.
	// if the volume does exist, a quick validity check is made on it, and then the result is opened
	// returns NULL if failure.  (permission denied to the file, or invalid filename passed, could be out of space... )
	// same as load_cyrypt_volume with userkey and devkey NULL.
	SACK_VFS_PROC struct volume * CPROC sack_vfs_fs_load_volume( CTEXTSTR filepath );

	// open a volume at the specified pathname.  Use the specified keys to encrypt it.
	// if the volume does not exist, will create it.
	// if the volume does exist, a quick validity check is made on it, and then the result is opened
	// returns NULL if failure.  (permission denied to the file, or invalid filename passed, could be out of space... )
	// if the keys are NULL same as load_volume.
	SACK_VFS_PROC struct volume * CPROC sack_vfs_fs_load_crypt_volume( CTEXTSTR filepath, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey );
	// pass some memory and a memory length of the memory to use as a volume.
	// if userkey and/or devkey are not NULL the memory is assume to be encrypted with those keys.
	// the space is opened as readonly; write accesses/expanding operations will fail.
	SACK_VFS_PROC struct volume * CPROC sack_vfs_fs_use_crypt_volume( POINTER filemem, size_t size, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey );
	// close a volume; release all resources; any open files will keep the volume open.
	// when the final file closes the volume will complete closing.
	SACK_VFS_PROC void            CPROC sack_vfs_fs_unload_volume( struct volume * vol );
	// remove unused extra allocated space at end of volume.  During working process, extra space is preallocated for
	// things to be stored in.
	SACK_VFS_PROC void            CPROC sack_vfs_fs_shrink_volume( struct volume * vol );
	// remove encryption from volume.
	SACK_VFS_PROC LOGICAL         CPROC sack_vfs_fs_decrypt_volume( struct volume *vol );
	// change the key applied to a volume.
	SACK_VFS_PROC LOGICAL         CPROC sack_vfs_fs_encrypt_volume( struct volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 );
	// create a signature of current directory of volume.
	// can be used to validate content.  Returns 256 character hex string.
	SACK_VFS_PROC const char *    CPROC sack_vfs_fs_get_signature( struct volume *vol );
	// pass an offset from memory start and the memory start...
	// computes the distance, uses that to generate a signature
	// returns BLOCK_SIZE length signature; recommend using at least 128 bits of it.
	SACK_VFS_PROC const uint8_t * CPROC sack_vfs_fs_get_signature2( POINTER disk, POINTER diskReal );

	// ---------- Operations on files in volumes ------------------

	// open a file, creates if does not exist.
	SACK_VFS_PROC struct sack_vfs_file * CPROC sack_vfs_fs_openfile( struct volume *vol, CTEXTSTR filename );
	// check if a file exists (if it does not exist, and you don't want it created, can use this and not openfile)
	SACK_VFS_PROC int CPROC sack_vfs_fs_exists( struct volume *vol, const char * file );
	// close a file.
	SACK_VFS_PROC int CPROC sack_vfs_fs_close( struct sack_vfs_file *file );
	// get the current File Position Index (FPI).
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_tell( struct sack_vfs_file *file );
	// get the length of the file
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_size( struct sack_vfs_file *file );
	// set the current File Position Index (FPI).
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_seek( struct sack_vfs_file *file, size_t pos, int whence );
	// write starting at the current FPI.
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_write( struct sack_vfs_file *file, const char * data, size_t length );
	// read starting at the current FPI.
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_read( struct sack_vfs_file *file, char * data, size_t length );
	// sets the file length to the current FPI.
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_truncate( struct sack_vfs_file *file );
	// psv should be struct volume *vol;
	// delete a filename.  Clear the space it was occupying.
	SACK_VFS_PROC int CPROC sack_vfs_fs_unlink_file( struct volume *vol, const char * filename );
	// rename a file within the filesystem; if the target name exists, it is deleted.  If the target file is also open, it will be prevented from deletion; and duplicate filenames will end up exising(?)
	SACK_VFS_PROC LOGICAL CPROC sack_vfs_fs_rename( uintptr_t psvInstance, const char *original, const char *newname );
	// -----------  directory interface commands. ----------------------

	// returns find_info which is then used in subsequent commands.
	SACK_VFS_PROC struct find_info * CPROC sack_vfs_fs_find_create_cursor( uintptr_t psvInst, const char *base, const char *mask );
	// reset find_info to the first directory entry.  returns 0 if no entry.
	SACK_VFS_PROC int CPROC sack_vfs_fs_find_first( struct find_info *info );
	// closes a find cursor; returns 0.
	SACK_VFS_PROC int CPROC sack_vfs_fs_find_close( struct find_info *info );
	// move to the next entry returns 0 if no entry.
	SACK_VFS_PROC int CPROC sack_vfs_fs_find_next( struct find_info *info );
	// get file information for the file at the current cursor position...
	SACK_VFS_PROC char * CPROC sack_vfs_fs_find_get_name( struct find_info *info );
	// get file information for the file at the current cursor position...
	SACK_VFS_PROC size_t CPROC sack_vfs_fs_find_get_size( struct find_info *info );

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace objStore {
#endif

	/* thse should probably be moved to sack_vfs_os.h being file system specific extensions. */
	enum sack_object_store_file_system_file_ioctl_ops {
		SOSFSFIO_PROVIDE_SEALANT,  // psvInstance should be a file handle pass (char*, size_t length )
		SOSFSFIO_TAMPERED, // test if file has been tampered, is is still sealed. pass (address of int)
		SOSFSFIO_STORE_OBJECT, // get the resulting storage ID.  (Move ID creation into low level driver)
		SOSFSFIO_PROVIDE_READKEY, // set key required to read this record.
		//SFSIO_GET_OBJECT_ID, // get the resulting storage ID.  (Move ID creation into low level driver)
	};

	enum sack_object_store_file_system_system_ioctl_ops {
		SOSFSSIO_STORE_OBJECT, // get the resulting storage ID.  (Move ID creation into low level driver)
		SOSFSSIO_PATCH_OBJECT,
		SOSFSSIO_LOAD_OBJECT,
		//SFSIO_GET_OBJECT_ID, // get the resulting storage ID.  (Move ID creation into low level driver)
	};

// returns a pointer to and array of buffers.
// the last pointer in the list is NULL.
// each pointer in the list points to a structure containing a pointer to the data and the length of the data
#define sack_vfs_os_ioctl_load_decrypt_object( vol, objId,objIdLen, seal,seallen )                            ((struct {uint8_t*, size_t}*)sack_fs_ioctl( vol, SOSFSSIO_LOAD_OBJECT, objId, objIdLen, seal, seallen ))

// returns a pointer to and array of buffers.
// the last pointer in the list is NULL.
// each pointer in the list points to a structure containing a pointer to the data and the length of the data
#define sack_vfs_os_ioctl_load_object( vol, objId,objIdLen )                                                  ((struct {uint8_t*, size_t}*)sack_fs_ioctl( vol, SOSFSSIO_LOAD_OBJECT, objId, objIdLen ))

// unsealed store/update(patch)
// returns TRUE/FALSE. true if the object already exists, or was successfully written.
// store object data, get a unique ID for the data.
// {
//     char data[] = "some data";
//     char result[44];
//     sack_vfs_os_ioctl_store_rw_object( vol, data, sizeof( data ), result, 44 );
// }
#define sack_vfs_os_ioctl_store_rw_object( vol, obj,objlen, result, resultlen )                                 sack_fs_ioctl( vol, SOSFSSIO_STORE_OBJECT, FALSE, FALSE, obj, objlen, NULL, 0, NULL, 0, result, resultlen )

// re-write an object with new content using old ID.
// returns TRUE/FALSE. true if the patch already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char oldResult[] = "AAAAAAAAAAAAAAAAAAAAAAAA"; // ID from previous store result
//     char result[44];
//     sack_vfs_os_ioctl_patch_rw_object( vol, oldResult, sizeof( oldReult-1 ), data, sizeof( data ), result, 44 );
// }
#define sack_vfs_os_ioctl_patch_rw_object( vol, objId,objIdLen, obj,objlen )                                     sack_fs_ioctl( vol, SOSFSSIO_PATCH_OBJECT, FALSE, FALSE, objId, objIdLen, NULL, 0, obj, objlen, NULL, 0, NULL, 0 )

// sealed store and patch
// store a unencrypted, sealed object using specified sealant

// store data to a new sealed block.  Also encrypt the data
// returns TRUE/FALSE. true if the object already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char result[44];
//     sack_vfs_os_ioctl_store_crypt_object( vol, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_store_crypt_owned_object( vol, obj,objlen, seal,seallen, readkey,readkeylen, result, resultlen )                 sack_fs_ioctl( vol, SOSFSSIO_STORE_OBJECT, TRUE,TRUE,  obj, objlen, NULL, 0, seal, seallen, readkey,readkeylen, result, resultlen )

// store data to a new sealed block.  Also encrypt the data
// returns TRUE/FALSE. true if the object already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char result[44];
//     sack_vfs_os_ioctl_store_crypt_object( vol, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_store_crypt_sealed_object( vol, obj,objlen, seal,seallen, readkey,readkeylen, result, resultlen )                 sack_fs_ioctl( vol, SOSFSSIO_STORE_OBJECT, TRUE,FALSE,  obj, objlen, NULL, 0, seal, seallen, readkey,readkeylen, result, resultlen )

// store patch to an existing sealed block.  (Writes never change existing data), also encrypt the data
// returns TRUE/FALSE. true if the patch already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char oldResult[] = "AAAAAAAAAAAAAAAAAAAAAAAA"; // ID from previous store result
//     char result[44];
//     sack_vfs_os_ioctl_patch_crypt_object( vol, oldResult, sizeof( oldResult )-1, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_patch_crypt_owned_object( vol, objId,objIdLen, obj,objlen, seal,seallen, readkey,readkeylen, result, resultlen ) sack_fs_ioctl( vol, SOSFSSIO_PATCH_OBJECT, TRUE, TRUE, objId, objIdLen, authId, authIdLen, obj, objlen, seal, seallen, readkey,readkeylen, result, resultlen )


// store patch to an existing sealed block.  (Writes never change existing data), also encrypt the data
// returns TRUE/FALSE. true if the patch already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char oldResult[] = "AAAAAAAAAAAAAAAAAAAAAAAA"; // ID from previous store result
//     char result[44];
//     sack_vfs_os_ioctl_patch_crypt_object( vol, oldResult, sizeof( oldResult )-1, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_patch_crypt_sealed_object( vol, objId,objIdLen, obj,objlen, seal,seallen, result, resultlen ) sack_fs_ioctl( vol, SOSFSSIO_PATCH_OBJECT, TRUE, FALSE, objId, objIdLen, authId, authIdLen, obj, objlen, seal, seallen, result, resultlen )

// store data to a new sealed block.  Data is publically readable.
// returns TRUE/FALSE. true if the object already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char result[44];
//     sack_vfs_os_ioctl_store_owned_object( vol, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_store_owned_object( vol, obj,objlen, seal,seallen, result, resultlen )                 sack_fs_ioctl( vol, SOSFSSIO_STORE_OBJECT, FALSE, TRUE, obj, objlen, NULL, 0, seal, seallen, NULL, 0, result, resultlen )

// store data to a new sealed block.  Data is publically readable.
// returns TRUE/FALSE. true if the object already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char result[44];
//     sack_vfs_os_ioctl_store_sealed_object( vol, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_store_sealed_object( vol, obj,objlen, seal,seallen, result, resultlen )                 sack_fs_ioctl( vol, SOSFSSIO_STORE_OBJECT, FALSE, FALSE, obj, objlen, NULL, 0, seal, seallen, NULL, 0, result, resultlen )

// store patch to an existing sealed block.  (Writes never change existing data).  Data is publically readable.
// returns TRUE/FALSE. true if the patch already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char oldResult[] = "AAAAAAAAAAAAAAAAAAAAAAAA"; // ID from previous store result
//     char result[44];
//     sack_vfs_os_ioctl_patch_object( vol, oldResult, sizeof( oldResult )-1, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_patch_owned_object( vol, objId,objIdLen, obj,objlen, seal,seallen, result, resultlen ) sack_fs_ioctl( vol, SOSFSSIO_PATCH_OBJECT, FALSE, TRUE, objId, objIdLen, authId, authIdLen, obj, objlen, seal, seallen, result, resultlen )

// store patch to an existing sealed block.  (Writes never change existing data).  Data is publically readable.
// returns TRUE/FALSE. true if the patch already exists, or was successfully written.
// {
//     char data[] = "some data";
//     char seal[] = "BBBBBBBBBBBBBBBBBBBBBBBB"; // Some sealant bsea64
//     char oldResult[] = "AAAAAAAAAAAAAAAAAAAAAAAA"; // ID from previous store result
//     char result[44];
//     sack_vfs_os_ioctl_patch_object( vol, oldResult, sizeof( oldResult )-1, data, sizeof( data ), seal, sizeof( seal ), result, 44 );
// }
#define sack_vfs_os_ioctl_patch_sealed_object( vol, objId,objIdLen, obj,objlen, seal,seallen, result, resultlen ) sack_fs_ioctl( vol, SOSFSSIO_PATCH_OBJECT, FALSE, FALSE, objId, objIdLen, authId, authIdLen, obj, objlen, seal, seallen, result, resultlen )

	struct volume;
	struct sack_vfs_file;
	struct find_info;
// open a volume at the specified pathname.
// if the volume does not exist, will create it.
// if the volume does exist, a quick validity check is made on it, and then the result is opened
// returns NULL if failure.  (permission denied to the file, or invalid filename passed, could be out of space... )
// same as load_cyrypt_volume with userkey and devkey NULL.
SACK_VFS_PROC struct volume * CPROC sack_vfs_os_load_volume( CTEXTSTR filepath );

// open a volume at the specified pathname.  Use the specified keys to encrypt it.
// if the volume does not exist, will create it.
// if the volume does exist, a quick validity check is made on it, and then the result is opened
// returns NULL if failure.  (permission denied to the file, or invalid filename passed, could be out of space... )
// if the keys are NULL same as load_volume.
SACK_VFS_PROC struct volume * CPROC sack_vfs_os_load_crypt_volume( CTEXTSTR filepath, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey );
// pass some memory and a memory length of the memory to use as a volume.
// if userkey and/or devkey are not NULL the memory is assume to be encrypted with those keys.
// the space is opened as readonly; write accesses/expanding operations will fail.
SACK_VFS_PROC struct volume * CPROC sack_vfs_os_use_crypt_volume( POINTER filemem, size_t size, uintptr_t version, CTEXTSTR userkey, CTEXTSTR devkey );
// close a volume; release all resources; any open files will keep the volume open.
// when the final file closes the volume will complete closing.
SACK_VFS_PROC void            CPROC sack_vfs_os_unload_volume( struct volume * vol );
// remove unused extra allocated space at end of volume.  During working process, extra space is preallocated for
// things to be stored in.
SACK_VFS_PROC void            CPROC sack_vfs_os_shrink_volume( struct volume * vol );
// remove encryption from volume.
SACK_VFS_PROC LOGICAL         CPROC sack_vfs_os_decrypt_volume( struct volume *vol );
// change the key applied to a volume.
SACK_VFS_PROC LOGICAL         CPROC sack_vfs_os_encrypt_volume( struct volume *vol, uintptr_t version, CTEXTSTR key1, CTEXTSTR key2 );
// create a signature of current directory of volume.
// can be used to validate content.  Returns 256 character hex string.
SACK_VFS_PROC const char *    CPROC sack_vfs_os_get_signature( struct volume *vol );
// pass an offset from memory start and the memory start...
// computes the distance, uses that to generate a signature
// returns BLOCK_SIZE length signature; recommend using at least 128 bits of it.
SACK_VFS_PROC const uint8_t * CPROC sack_vfs_os_get_signature2( POINTER disk, POINTER diskReal );

// ---------- Operations on files in volumes ------------------

// open a file, creates if does not exist.
SACK_VFS_PROC struct sack_vfs_file * CPROC sack_vfs_os_openfile( struct volume *vol, CTEXTSTR filename );
// check if a file exists (if it does not exist, and you don't want it created, can use this and not openfile)
SACK_VFS_PROC int CPROC sack_vfs_os_exists( struct volume *vol, const char * file );
// close a file.
SACK_VFS_PROC int CPROC sack_vfs_os_close( struct sack_vfs_file *file );
// get the current File Position Index (FPI).
SACK_VFS_PROC size_t CPROC sack_vfs_os_tell( struct sack_vfs_file *file );
// get the length of the file
SACK_VFS_PROC size_t CPROC sack_vfs_os_size( struct sack_vfs_file *file );
// set the current File Position Index (FPI).
SACK_VFS_PROC size_t CPROC sack_vfs_os_seek( struct sack_vfs_file *file, size_t pos, int whence );
// write starting at the current FPI.
SACK_VFS_PROC size_t CPROC sack_vfs_os_write( struct sack_vfs_file *file, const char * data, size_t length );
// read starting at the current FPI.
SACK_VFS_PROC size_t CPROC sack_vfs_os_read( struct sack_vfs_file *file, char * data, size_t length );
// sets the file length to the current FPI.
SACK_VFS_PROC size_t CPROC sack_vfs_os_truncate( struct sack_vfs_file *file );
// psv should be struct volume *vol;
// delete a filename.  Clear the space it was occupying.
SACK_VFS_PROC int CPROC sack_vfs_os_unlink_file( struct volume *vol, const char * filename );
// rename a file within the filesystem; if the target name exists, it is deleted.  If the target file is also open, it will be prevented from deletion; and duplicate filenames will end up exising(?)
SACK_VFS_PROC LOGICAL CPROC sack_vfs_os_rename( uintptr_t psvInstance, const char *original, const char *newname );
// -----------  directory interface commands. ----------------------

// returns find_info which is then used in subsequent commands.
SACK_VFS_PROC struct find_info * CPROC sack_vfs_os_find_create_cursor( uintptr_t psvInst, const char *base, const char *mask );
// reset find_info to the first directory entry.  returns 0 if no entry.
SACK_VFS_PROC int CPROC sack_vfs_os_find_first( struct find_info *info );
// closes a find cursor; returns 0.
SACK_VFS_PROC int CPROC sack_vfs_os_find_close( struct find_info *info );
// move to the next entry returns 0 if no entry.
SACK_VFS_PROC int CPROC sack_vfs_os_find_next( struct find_info *info );
// get file information for the file at the current cursor position...
SACK_VFS_PROC char * CPROC sack_vfs_os_find_get_name( struct find_info *info );
// get file information for the file at the current cursor position...
SACK_VFS_PROC size_t CPROC sack_vfs_os_find_get_size( struct find_info *info );

#ifdef __cplusplus
}
#endif

#if defined USE_VFS_FS_INTERFACE

#define sack_vfs_load_volume  sack_vfs_fs_load_volume
#define sack_vfs_load_crypt_volume  sack_vfs_fs_load_crypt_volume
#define sack_vfs_use_crypt_volume  sack_vfs_fs_use_crypt_volume
#define sack_vfs_unload_volume  sack_vfs_fs_unload_volume
#define sack_vfs_shrink_volume  sack_vfs_fs_shrink_volume
#define sack_vfs_decrypt_volume  sack_vfs_fs_decrypt_volume
#define sack_vfs_encrypt_volume  sack_vfs_fs_encrypt_volume
#define sack_vfs_get_signature  sack_vfs_fs_get_signature
#define sack_vfs_get_signature2  sack_vfs_fs_get_signature2


#define sack_vfs_openfile  sack_vfs_fs_openfile
#define sack_vfs_exists  sack_vfs_fs_exists
#define sack_vfs_close  sack_vfs_fs_close
#define sack_vfs_tell  sack_vfs_fs_tell
#define sack_vfs_size  sack_vfs_fs_size
#define sack_vfs_seek  sack_vfs_fs_seek
#define sack_vfs_write  sack_vfs_fs_write
#define sack_vfs_read  sack_vfs_fs_read
#define sack_vfs_truncate  sack_vfs_fs_truncate
#define sack_vfs_unlink_file  sack_vfs_fs_unlink_file
#define sack_vfs_rename  sack_vfs_fs_rename
#define sack_vfs_find_create_cursor  sack_vfs_fs_find_create_cursor
#define sack_vfs_find_first  sack_vfs_fs_find_first
#define sack_vfs_find_close  sack_vfs_fs_find_close
#define sack_vfs_find_next  sack_vfs_fs_find_next
#define sack_vfs_find_get_name  sack_vfs_fs_find_get_name
#define sack_vfs_find_get_size  sack_vfs_fs_find_get_size

#endif


#if defined USE_VFS_OS_INTERFACE

#define sack_vfs_load_volume  sack_vfs_os_load_volume
#define sack_vfs_load_crypt_volume  sack_vfs_os_load_crypt_volume
#define sack_vfs_use_crypt_volume  sack_vfs_os_use_crypt_volume
#define sack_vfs_unload_volume  sack_vfs_os_unload_volume
#define sack_vfs_shrink_volume  sack_vfs_os_shrink_volume
#define sack_vfs_decrypt_volume  sack_vfs_os_decrypt_volume
#define sack_vfs_encrypt_volume  sack_vfs_os_encrypt_volume
#define sack_vfs_get_signature  sack_vfs_os_get_signature
#define sack_vfs_get_signature2  sack_vfs_os_get_signature2


#define sack_vfs_openfile  sack_vfs_os_openfile
#define sack_vfs_exists  sack_vfs_os_exists
#define sack_vfs_close  sack_vfs_os_close
#define sack_vfs_tell  sack_vfs_os_tell
#define sack_vfs_size  sack_vfs_os_size
#define sack_vfs_seek  sack_vfs_os_seek
#define sack_vfs_write  sack_vfs_os_write
#define sack_vfs_read  sack_vfs_os_read
#define sack_vfs_truncate  sack_vfs_os_truncate
#define sack_vfs_unlink_file  sack_vfs_os_unlink_file
#define sack_vfs_rename  sack_vfs_os_rename
#define sack_vfs_find_create_cursor  sack_vfs_os_find_create_cursor
#define sack_vfs_find_first  sack_vfs_os_find_first
#define sack_vfs_find_close  sack_vfs_os_find_close
#define sack_vfs_find_next  sack_vfs_os_find_next
#define sack_vfs_find_get_name  sack_vfs_os_find_get_name
#define sack_vfs_find_get_size  sack_vfs_os_find_get_size

#endif



SACK_VFS_NAMESPACE_END
#if defined( __cplusplus ) && !defined( SACK_VFS_SOURCE )
using namespace sack::SACK_VFS;
//using namespace sack::SACK_VFS::fs;
//using namespace sack::SACK_VFS::objStore;
#endif
#endif
