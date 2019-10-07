/**
  * @file    DataManager.cpp
  * @version 0.1.0
  * @author  Rafaella Neofytou, Adam Mitchell
  * @brief   C++ file of the DataManager. Provides a very lightweight filesystem to facilitate the
  *          storage of arbritrary file types
  */

/** Includes
 */
#include "DataManager.h"


#if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0)
DataManager::DataManager(PinName write_control, PinName sda, PinName scl, int frequency_hz) : 
                         _storage(write_control, sda, scl, frequency_hz)
{
    
}
#endif /* #if defined (BOARD) && (BOARD == DEVELOPMENT_BOARD_V1_1_0) */


DataManager::~DataManager()
{
    #if defined (_PERSISTENT_STORAGE_DRIVER) && (_PERSISTENT_STORAGE_DRIVER == STM24256)
    _storage.~STM24256();
    #endif /* #if defined (_PERSISTENT_STORAGE_DRIVER) && (_PERSISTENT_STORAGE_DRIVER == STM24256) */
}

/** Return maximum number of file type definitions that can be stored
 *  in persistent storage
 *
 *  @return Maximum number of file type definitions that can be stored
 */
uint16_t DataManager::get_max_types()
{
	return (uint16_t)TYPE_STORE_LENGTH / sizeof(DataManager_FileSystem::FileType_t);
}

/** Return maximum number of file records that can be stored
 *  in persistent storage
 *
 *  @return Maximum number of file records that can be stored
 */
int DataManager::get_max_records()
{
	return (int)RECORD_STORE_LENGTH / sizeof(DataManager_FileSystem::FileRecord_t);
}

/** Return overall total file storage size in bytes
 *
 *  @return Total usable space, in bytes, for file storage
 */
int DataManager::get_storage_size_bytes()
{
	return (int)STORAGE_LENGTH;
}

/** Initialise the file type table to all zeros
 *
 * @return Indicates success or failure reason
 */
int DataManager::initialise_type_table()
{
    char blank[TYPE_STORE_PAGES * PAGE_SIZE_BYTES] = { 0 };

    int status = _storage.write_to_address(TYPE_STORE_START_ADDRESS, blank, TYPE_STORE_PAGES * PAGE_SIZE_BYTES);
    
    if(status != DataManager::DATA_MANAGER_OK) 
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Initialise the file record table to all zeros
 *
 * @return Indicates success or failure reason
 */
int DataManager::init_record_table()
{

}

