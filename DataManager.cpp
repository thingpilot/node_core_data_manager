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
 * @param type FileType_t object representing file type definition to be
 *             stored into persistent storage medium
 * @return Indicates success or failure reason
 */
int DataManager::add_file_type(DataManager_FileSystem::FileType_t type)
{
    type.parameters.valid = type.parameters.type_id + type.parameters.length_bytes;

    int address = -1;
    int next_address_status = get_next_available_file_type_table_address(address);

    if(next_address_status != DataManager::DATA_MANAGER_OK) 
    {
        return next_address_status;
    }

    if(address == -1)
    {
        return DataManager::FILE_TYPE_TABLE_FULL;
    }

    int write_status = _storage.write_to_address(address, type.data, sizeof(type));

    if(write_status != DataManager::DATA_MANAGER_OK)
    {
        return write_status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Add new file record entry to the file record table
 *
 * @param record FileRecord_t object representing file record to be
 *               stored into persistent storage medium
 * @return Indicates success or failure reason
 */
int DataManager::add_file_record(DataManager_FileSystem::FileRecord_t record)
{
    record.parameters.valid = (record.parameters.start_address + record.parameters.length_bytes + 
                               record.parameters.record_id + record.parameters.type_id);

    int address = -1;
    int next_address_status = get_next_available_file_record_table_address(address);

    if(next_address_status != DataManager::DATA_MANAGER_OK)
    {
        return next_address_status;
    }

    if(address == -1)
    {
        return DataManager::FILE_RECORD_TABLE_FULL;
    }

    int write_status = _storage.write_to_address(address, record.data, sizeof(record));

    if(write_status != DataManager::DATA_MANAGER_OK)
    {
        return write_status;
    }

    return DataManager::DATA_MANAGER_OK;
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

/** Perform checksum on given FileRecord_t using the 'valid' parameter
 *
 * @param record File record to be checked for validity
 * @return True if file record entry is valid, else false
 */
bool DataManager::is_valid_file_record(DataManager_FileSystem::FileRecord_t record)
{
    if(record.parameters.valid == 0x00)
    {
        return false;
    }

    uint32_t checksum = (record.parameters.start_address + record.parameters.length_bytes + 
                         record.parameters.record_id + record.parameters.type_id) & 0x000000FF;

    if(record.parameters.valid != checksum)
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

/** Calculate the number of valid file type records currently 
 *  stored in memory
 * @param &valid_entries Address of integer value in which number of 
 *                       detected valid entries will be stored
 * @return Indicates success or failure reason                        
 */
int DataManager::total_stored_file_record_entries(int &valid_entries)
{
    DataManager_FileSystem::FileRecord_t record;
    int record_size = sizeof(record);

    int max_records = get_max_records();

    for(uint16_t record_index = 0; record_index < max_records; record_index++)
    {
        int status = _storage.read_from_address(RECORD_STORE_START_ADDRESS + (record_index * record_size), record.data, record_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(is_valid_file_record(record))
        {
            valid_entries++;
        }
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate total number of spaces available in the file type definition table
 *  for new entries
 *
 * @param &remaining_entries Address of integer value in which the total number
 *                           of spaces available in the file type table is to be
 *                           written
 * @return Indicates success or failure reason
 */
int DataManager::total_remaining_file_type_entries(int &remaining_entries)
{
    int valid_entries = 0;
    int status = total_stored_file_type_entries(valid_entries);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status; 
    }

    uint16_t max_types = get_max_types();

    remaining_entries = max_types - valid_entries;

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate total number of spaces available in the file record table
 *  for new entries
 *
 * @param &remaining_entries Address of integer value in which the total number
 *                           of spaces available in the file record table is to be
 *                           written
 * @return Indicates success or failure reason
 */
int DataManager::total_remaining_file_record_entries(int &remaining_entries)
{
    int valid_entries = 0;
    int status = total_stored_file_record_entries(valid_entries);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    int max_records = get_max_records();

    remaining_entries = max_records - valid_entries;

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

/** Determine the next available address to which to write file records
 *
 * @param &next_available_address Address of integer value in which the address
 *                                of the next available location in memory to which
 *                                you can write a file record entry is stored. -1 if 
 *                                there are no available spaces
 * @return Indicates success or failure reason
 */
int DataManager::get_next_available_file_record_table_address(int &next_available_address)
{
    DataManager_FileSystem::FileRecord_t record;
    int record_size = sizeof(record);

    int max_records = get_max_records();

    for(int record_index = 0; record_index < max_records; record_index++)
    {
        int address = RECORD_STORE_START_ADDRESS + (record_index * record_size);
        int status = _storage.read_from_address(address, record.data, record_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file_record(record))
        {
            next_available_address = address;
            break;
        }
    }

    return DataManager::DATA_MANAGER_OK;
}

