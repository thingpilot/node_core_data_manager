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

/** Initialise the file type and record tables to all zeros
 *
 * @return Indicates success or failure reason
 */
int DataManager::init_filesystem()
{
    char blank[PAGE_SIZE_BYTES] = { 0 };

    int status;

    for(int ts_page = 0; ts_page < TYPE_STORE_PAGES; ts_page++)
    {
        status = _storage.write_to_address(TYPE_STORE_START_ADDRESS + (ts_page * PAGE_SIZE_BYTES), blank, PAGE_SIZE_BYTES);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        wait_us(5000);
    }

    for(int rs_page = 0; rs_page < RECORD_STORE_PAGES; rs_page++)
    {
        status = _storage.write_to_address(RECORD_STORE_START_ADDRESS + (rs_page * PAGE_SIZE_BYTES), blank, PAGE_SIZE_BYTES);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        wait_us(5000);
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Add new file type entry to the file type table
 *
 * @param type_id Enumerated value of the file type to be added
 * @param length_bytes Size of the sum of the struct's components,
 *                     equivalent to sizeof(yourStruct), in bytes
 * @return Indicates success or failure reason
 */
int DataManager::add_file_type(uint8_t type_id, uint16_t length_bytes)
{
    DataManager_FileSystem::FileType_t type;
    type.parameters.type_id = type_id;
    type.parameters.length_bytes = length_bytes;
    type.parameters.valid = type_id + length_bytes;

    _storage.write_to_address(16, type.data, 4);

    return 0;
}

/** Perform checksum on given FileType_t using the 'valid' parameter
 *
 * @param type File type defintion to be checked for validity
 * @return True if file type entry is valid, else false
 */
bool DataManager::is_valid_file_type(DataManager_FileSystem::FileType_t type)
{
    /** During init_filesystem() we set every bit in the type and record tables to 0
     *  so, if the valid byte == 0, this can't be a valid entry
     */
    if(type.parameters.valid == 0x00)
    {
        return false;
    }
    
    /** Mask the first 24 bits so that we can use our 8-bit valid flag as a rudimentary checksum 
     *  of the length and type id
     */
    uint32_t checksum = (type.parameters.length_bytes + type.parameters.type_id) & 0x000000FF;

    if(type.parameters.valid != checksum)
    {
        return false;
    }


    return true;
}

/** Calculate the number of valid file type definitions currently 
 *  stored in memory
 * @param &valid_entries Address of integer value in which number of 
 *                       detected valid entries will be stored
 * @return Indicates success or failure reason                        
 */
int DataManager::total_stored_file_type_entries(int &valid_entries)
{
    DataManager_FileSystem::FileType_t type;
    int type_size = sizeof(type);
    
    uint16_t max_types = get_max_types();

    for(uint16_t type_index = 0; type_index < max_types; type_index++)
    {
        int status = _storage.read_from_address(TYPE_STORE_START_ADDRESS + (type_index * type_size), type.data, type_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(is_valid_file_type(type))
        {
            valid_entries++;
        }
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Determine the next available address to which to write file type definition
 *
 * @param &next_available_address Address of integer value in which the address
 *                                of the next available location in memory to which
 *                                you can write a file type entry is stored. -1 if 
 *                                there are no available spaces
 * @return Indicates success or failure reason
 */
int DataManager::get_next_available_file_type_table_address(int &next_available_address)
{
    DataManager_FileSystem::FileType_t type;
    int type_size = sizeof(type);
    
    uint16_t max_types = get_max_types();

    next_available_address = -1;

    for(uint16_t type_index = 0; type_index < max_types; type_index++)
    {
        int address = TYPE_STORE_START_ADDRESS + (type_index * type_size);
        int status = _storage.read_from_address(address, type.data, type_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file_type(type))
        {
            next_available_address = address;
            break;
        }
    }

    return DataManager::DATA_MANAGER_OK;
}

