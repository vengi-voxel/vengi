#pragma once

#include "PolyVox/PagedVolume.h"
#include "PolyVox/Region.h"
#include "core/Common.h"
#include "core/String.h"

#include "sqlite3.h"
#include "zlib.h"

#include "WritePermissions.h"

#include <vector>
#include "PolyVox/Timer.h"
#include "PolyVox/Utility.h"

#include "SQLiteUtils.h"

#include <climits>

namespace Cubiquity {

/**
 * Provides an interface for performing paging of data.
 */
template<typename VoxelType>
class VoxelDatabase: public PolyVox::PagedVolume<VoxelType>::Pager {
public:
	/// Destructor
	virtual ~VoxelDatabase();

	static VoxelDatabase* createEmpty(const std::string& pathToNewVoxelDatabase);
	static VoxelDatabase* createFromVDB(const std::string& pathToExistingVoxelDatabase, WritePermission writePermission);

	virtual void pageIn(const PolyVox::Region& region, typename PolyVox::PagedVolume<VoxelType>::Chunk* pChunk);
	virtual void pageOut(const PolyVox::Region& region, typename PolyVox::PagedVolume<VoxelType>::Chunk* pChunk);

	void acceptOverrideChunks(void);
	void discardOverrideChunks(void);

	int32_t getPropertyAsInt(const std::string& name, int32_t defaultValue);
	float getPropertyAsFloat(const std::string& name, float defaultValue);
	std::string getPropertyAsString(const std::string& name, const std::string& defaultValue);

	void setProperty(const std::string& name, int value);
	void setProperty(const std::string& name, float value);
	void setProperty(const std::string& name, const std::string& value);

private:
	VoxelDatabase();

	void initialize(void);

	bool getProperty(const std::string& name, std::string& value);

	sqlite3* _database = nullptr;

	sqlite3_stmt* _selectChunkStatement = nullptr;
	sqlite3_stmt* _selectOverrideChunkStatement = nullptr;

	sqlite3_stmt* _insertOrReplaceBlockStatement = nullptr;
	sqlite3_stmt* _insertOrReplaceOverrideChunkStatement = nullptr;

	sqlite3_stmt* _selectPropertyStatement = nullptr;
	sqlite3_stmt* _insertOrReplacePropertyStatement = nullptr;

	// Used as a temporary store into which we compress
	// chunk data, before passing it to the database.
	std::vector<uint8_t> _compressedBuffer;
};

// Utility function to perform bit rotation.
template<typename T>
T rotateLeft(T val);

// Allows us to use a Region as a key in the SQLite database.
uint64_t regionToKey(const PolyVox::Region& region);

// From http://stackoverflow.com/a/776550
// Should only be used on unsigned types.
template<typename T>
T rotateLeft(T val) {
	return (val << 1) | (val >> (sizeof(T) * CHAR_BIT - 1));
}

/// Constructor
template<typename VoxelType>
VoxelDatabase<VoxelType>::VoxelDatabase() :
		PolyVox::PagedVolume<VoxelType>::Pager() {
}

/// Destructor
template<typename VoxelType>
VoxelDatabase<VoxelType>::~VoxelDatabase() {
	EXECUTE_SQLITE_FUNC(sqlite3_finalize(_selectChunkStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_finalize(_selectOverrideChunkStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_finalize(_insertOrReplaceBlockStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_finalize(_insertOrReplaceOverrideChunkStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_finalize(_selectPropertyStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_finalize(_insertOrReplacePropertyStatement));

	if (sqlite3_db_readonly(_database, "main") == 0) {
		Log::trace("Vacuuming database...");
		PolyVox::Timer timer;
		EXECUTE_SQLITE_FUNC(sqlite3_exec(_database, "VACUUM;", 0, 0, 0));
		Log::trace("Vacuumed database in %f ms", timer.elapsedTimeInMilliSeconds());
	}

	EXECUTE_SQLITE_FUNC(sqlite3_close(_database));
}

template<typename VoxelType>
VoxelDatabase<VoxelType>* VoxelDatabase<VoxelType>::createEmpty(const std::string& pathToNewVoxelDatabase) {
	// Make sure that the provided path doesn't already exist.
	// If the file is NULL then we don't need to (and can't) close it.
	FILE* file = fopen(pathToNewVoxelDatabase.c_str(), "rb");
	if (file != NULL) {
		fclose(file);
		core_assert_msg(false, "Cannot create a new voxel database as the provided filename - already exists");
	}

	POLYVOX_LOG_INFO("Creating empty voxel database as '", pathToNewVoxelDatabase, "'");
	VoxelDatabase<VoxelType>* voxelDatabase = new VoxelDatabase<VoxelType>;
	EXECUTE_SQLITE_FUNC(sqlite3_open_v2(pathToNewVoxelDatabase.c_str(), &(voxelDatabase->_database), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL));

	// Create the 'Properties' table.
	EXECUTE_SQLITE_FUNC(sqlite3_exec(voxelDatabase->_database, "CREATE TABLE Properties(Name TEXT PRIMARY KEY, Value TEXT);", 0, 0, 0));

	// Create the 'Blocks' table. Not sure we need 'ASC' here, but it's in the example (http://goo.gl/NLHjQv) as is the default anyway.
	EXECUTE_SQLITE_FUNC(sqlite3_exec(voxelDatabase->_database, "CREATE TABLE Blocks(Region INTEGER PRIMARY KEY ASC, Data BLOB);", 0, 0, 0));

	voxelDatabase->initialize();
	return voxelDatabase;
}

template<typename VoxelType>
VoxelDatabase<VoxelType>* VoxelDatabase<VoxelType>::createFromVDB(const std::string& pathToExistingVoxelDatabase, WritePermission writePermission) {
	// When creating a new empty voxel database the user can pass an empty string to signify that
	// the database will be temporary, but when creating from a VDB a valid path must be provided.
	core_assert_msg(!pathToExistingVoxelDatabase.empty(), "Path must not be an empty string");

	POLYVOX_LOG_INFO("Creating voxel database from '", pathToExistingVoxelDatabase, "'");
	VoxelDatabase<VoxelType>* voxelDatabase = new VoxelDatabase<VoxelType>;
	int flags = writePermission == WritePermissions::ReadOnly ? SQLITE_OPEN_READONLY : SQLITE_OPEN_READWRITE;
	EXECUTE_SQLITE_FUNC(sqlite3_open_v2(pathToExistingVoxelDatabase.c_str(), &(voxelDatabase->_database), flags, NULL));

	// If the database was requested with write permissions but only read-only was possible, then SQLite opens it in read-only mode
	// instead. This is undisirable for us as we would rather know that it has failed. In one case this was due to a user having the
	// VDB in source control, and is is desirable to give an error so that the user knows they need to check out the database.
	if ((writePermission == WritePermissions::ReadWrite) && (sqlite3_db_readonly(voxelDatabase->_database, "main") == 1)) {
		EXECUTE_SQLITE_FUNC(sqlite3_close(voxelDatabase->_database));
		core_assert_msg(false, "Voxel database could not be opened with requested 'write' permissions (only read-only was possible)");
	}

	voxelDatabase->initialize();
	return voxelDatabase;
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::initialize(void) {
	// Disable syncing
	EXECUTE_SQLITE_FUNC(sqlite3_exec(_database, "PRAGMA synchronous = OFF", 0, 0, 0));

	// Now create the 'OverrideChunks' table. Not sure we need 'ASC' here, but it's in the example (http://goo.gl/NLHjQv) and is the default anyway.
	// Note that the table cannot already exist because it's created as 'TEMP', and is therefore stored in a seperate temporary database.
	// It appears this temporary table is not shared between connections (multiple volumes using the same VDB) which is probably desirable for us
	// as it means different instances of the volume can be modified (but not commited to) without interfering with each other (http://goo.gl/aDKyId).
	EXECUTE_SQLITE_FUNC(sqlite3_exec(_database, "CREATE TEMP TABLE OverrideChunks(Region INTEGER PRIMARY KEY ASC, Data BLOB);", 0, 0, 0));

	// Now build the 'insert or replace' prepared statements
	EXECUTE_SQLITE_FUNC(sqlite3_prepare_v2(_database, "INSERT OR REPLACE INTO Blocks (Region, Data) VALUES (?, ?)", -1, &_insertOrReplaceBlockStatement, NULL));
	EXECUTE_SQLITE_FUNC(sqlite3_prepare_v2(_database, "INSERT OR REPLACE INTO OverrideChunks (Region, Data) VALUES (?, ?)", -1, &_insertOrReplaceOverrideChunkStatement, NULL));

	// Now build the 'select' prepared statements
	EXECUTE_SQLITE_FUNC(sqlite3_prepare_v2(_database, "SELECT Data FROM Blocks WHERE Region = ?", -1, &_selectChunkStatement, NULL));
	EXECUTE_SQLITE_FUNC(sqlite3_prepare_v2(_database, "SELECT Data FROM OverrideChunks WHERE Region = ?", -1, &_selectOverrideChunkStatement, NULL));

	// Now build the 'select' and 'insert or replace' prepared statements
	EXECUTE_SQLITE_FUNC(sqlite3_prepare_v2(_database, "SELECT Value FROM Properties WHERE Name = ?", -1, &_selectPropertyStatement, NULL));
	EXECUTE_SQLITE_FUNC(sqlite3_prepare_v2(_database, "INSERT OR REPLACE INTO Properties (Name, Value) VALUES (?, ?)", -1, &_insertOrReplacePropertyStatement, NULL));
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::pageIn(const PolyVox::Region& region, typename PolyVox::PagedVolume<VoxelType>::Chunk* pChunk) {
	core_assert_msg(pChunk != nullptr, "Attempting to page in NULL chunk");

	PolyVox::Timer timer;

	int64_t key = regionToKey(region);

	const void* compressedData = nullptr;
	int compressedLength = 0;

	// First we try and read the data from the OverrideChunks table
	// Based on: http://stackoverflow.com/a/5308188
	sqlite3_reset(_selectOverrideChunkStatement);
	sqlite3_bind_int64(_selectOverrideChunkStatement, 1, key);
	if (sqlite3_step(_selectOverrideChunkStatement) == SQLITE_ROW) {
		// I think the last index is zero because our select statement only returned one column.
		compressedLength = sqlite3_column_bytes(_selectOverrideChunkStatement, 0);
		compressedData = sqlite3_column_blob(_selectOverrideChunkStatement, 0);
	} else {
		// In this case the chunk data wasn't found in the override table, so we go to the real Chunks table.
		sqlite3_reset(_selectChunkStatement);
		sqlite3_bind_int64(_selectChunkStatement, 1, key);
		if (sqlite3_step(_selectChunkStatement) == SQLITE_ROW) {
			// I think the last index is zero because our select statement only returned one column.
			compressedLength = sqlite3_column_bytes(_selectChunkStatement, 0);
			compressedData = sqlite3_column_blob(_selectChunkStatement, 0);
		}
	}

	// The data might not have been found in the database, in which case
	// we leave the chunk in it's default state (initialized to zero).
	if (compressedData) {
		uLongf uncomp_len = pChunk->getDataSizeInBytes();
		int status = uncompress((unsigned char*) pChunk->getData(), &uncomp_len, (const unsigned char*) compressedData, compressedLength);
		core_assert_msg(status == Z_OK, "Decompression failed");

		// Data on disk is stored in linear order because so far we have not been able to show that Morton order
		// has better compression. But data in memory has Morton order because it is (probably) faster to access.
		pChunk->changeLinearOrderingToMorton();
	}

	Log::trace("Paged chunk in in %f ms", timer.elapsedTimeInMilliSeconds());
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::pageOut(const PolyVox::Region& region, typename PolyVox::PagedVolume<VoxelType>::Chunk* pChunk) {
	core_assert_msg(pChunk != nullptr, "Attempting to page out NULL chunk");

	PolyVox::Timer timer;

	POLYVOX_LOG_TRACE("Paging out data for ", region);

	// Data on disk is stored in linear order because so far we have not been able to show that Morton order
	// has better compression. But data in memory has Morton order because it is (probably) faster to access.
	pChunk->changeMortonOrderingToLinear();

	// Prepare for compression
	uLong srcLength = pChunk->getDataSizeInBytes();
	uLong compressedLength = compressBound(srcLength); // Gets update when compression happens
	if (_compressedBuffer.size() != compressedLength) {
		// All chunks are the same size so should have the same upper bound. Therefore this should only happen once.
		POLYVOX_LOG_INFO("Resizing compressed data buffer to ", compressedLength, "bytes. This should only happen once");
		_compressedBuffer.resize(compressedLength);
	}

	// Perform the compression, and update passed parameter with the new length.
	int status = compress(&(_compressedBuffer[0]), &compressedLength, (const unsigned char *) pChunk->getData(), srcLength);
	core_assert_msg(status == Z_OK, "Compression failed");

	int64_t key = regionToKey(region);

	// Based on: http://stackoverflow.com/a/5308188
	sqlite3_reset(_insertOrReplaceOverrideChunkStatement);
	sqlite3_bind_int64(_insertOrReplaceOverrideChunkStatement, 1, key);
	sqlite3_bind_blob(_insertOrReplaceOverrideChunkStatement, 2, static_cast<const void*>(&(_compressedBuffer[0])), compressedLength, SQLITE_TRANSIENT);
	sqlite3_step(_insertOrReplaceOverrideChunkStatement);

	Log::trace("Paged chunk out in %f ms (%i bytes of data)", timer.elapsedTimeInMilliSeconds(), pChunk->getDataSizeInBytes());
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::acceptOverrideChunks(void) {
	EXECUTE_SQLITE_FUNC(sqlite3_exec(_database, "INSERT OR REPLACE INTO Blocks (Region, Data) SELECT Region, Data from OverrideChunks;", 0, 0, 0));

	// The override chunks have been copied accross so we
	// can now discard the contents of the override table.
	discardOverrideChunks();
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::discardOverrideChunks(void) {
	EXECUTE_SQLITE_FUNC(sqlite3_exec(_database, "DELETE FROM OverrideChunks;", 0, 0, 0));
}

template<typename VoxelType>
bool VoxelDatabase<VoxelType>::getProperty(const std::string& name, std::string& value) {
	EXECUTE_SQLITE_FUNC(sqlite3_reset(_selectPropertyStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_bind_text(_selectPropertyStatement, 1, name.c_str(), -1, SQLITE_TRANSIENT));
	if (sqlite3_step(_selectPropertyStatement) == SQLITE_ROW) {
		// I think the last index is zero because our select statement only returned one column.
		value = std::string(reinterpret_cast<const char*>(sqlite3_column_text(_selectPropertyStatement, 0)));
		return true;
	}
	POLYVOX_LOG_WARNING("Property '", name, "' was not found. The default value will be used instead");
	return false;
}

template<typename VoxelType>
int32_t VoxelDatabase<VoxelType>::getPropertyAsInt(const std::string& name, int32_t defaultValue) {
	std::string value;
	if (getProperty(name, value)) {
		return core::string::toInt(value);
	}

	return defaultValue;
}

template<typename VoxelType>
float VoxelDatabase<VoxelType>::getPropertyAsFloat(const std::string& name, float defaultValue) {
	std::string value;
	if (getProperty(name, value)) {
		return core::string::toFloat(value);
	}

	return defaultValue;
}

template<typename VoxelType>
std::string VoxelDatabase<VoxelType>::getPropertyAsString(const std::string& name, const std::string& defaultValue) {
	std::string value;
	if (getProperty(name, value)) {
		return value;
	}

	return defaultValue;
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::setProperty(const std::string& name, int value) {
	setProperty(name, std::to_string(value));
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::setProperty(const std::string& name, float value) {
	setProperty(name, std::to_string(value));
}

template<typename VoxelType>
void VoxelDatabase<VoxelType>::setProperty(const std::string& name, const std::string& value) {
	// Based on: http://stackoverflow.com/a/5308188
	EXECUTE_SQLITE_FUNC(sqlite3_reset(_insertOrReplacePropertyStatement));
	EXECUTE_SQLITE_FUNC(sqlite3_bind_text(_insertOrReplacePropertyStatement, 1, name.c_str(), -1, SQLITE_TRANSIENT));
	EXECUTE_SQLITE_FUNC(sqlite3_bind_text(_insertOrReplacePropertyStatement, 2, value.c_str(), -1, SQLITE_TRANSIENT));
	sqlite3_step(_insertOrReplacePropertyStatement); //Don't wrap this one as it isn't supposed to return SQLITE_OK?
}

}
