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

    DataManager_FileSystem::GlobalStats_t g_stats;
    g_stats.parameters.next_available_address = STORAGE_START_ADDRESS;

    int max_storage_size = get_storage_size_bytes();
    g_stats.parameters.space_remaining = max_storage_size;

    status = set_global_stats(g_stats.data);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}


int DataManager::set_global_stats(char *data)
{
    int status = _storage.write_to_address(GLOBAL_STATS_START_ADDRESS, data, GLOBAL_STATS_LENGTH);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}


int DataManager::get_global_stats(char *data)
{
    int status = _storage.read_from_address(GLOBAL_STATS_START_ADDRESS, data, GLOBAL_STATS_LENGTH);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/*
int DataManager::write_data(uint8_t type_id, char *data)
{
    DataManager_FileSystem::FileType_t type;

    int get_type_status = get_file_type_by_type_id(type_id, type);

    if(get_type_status != DataManager::DATA_MANAGER_OK)
    {
        return get_type_status;
    }

    DataManager_FileSystem::FileRecord_t record;

}
*/

/** Get all FileType_t parameters for a given type_id
 *
 * @param type_id ID of file type definition to be retrieved
 * @param &type Address of FileType_t object in which retrieved information
 *              will be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_file_type_by_id(uint8_t type_id, DataManager_FileSystem::FileType_t &type)
{
    int type_size = sizeof(DataManager_FileSystem::FileType_t);

    uint16_t max_types = get_max_types();
    bool match = false;

    for(uint16_t type_index = 0; type_index < max_types; type_index++)
    {
        int status = _storage.read_from_address(TYPE_STORE_START_ADDRESS + (type_index * type_size), type.data, type_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file_type(type))
        {
            continue;
        }

        if(type_id == type.parameters.type_id)
        {
            match = true;
            break;
        }
    }

    if(!match)
    {
        return DataManager::DATA_MANAGER_INVALID_TYPE;
    }
    
    return DataManager::DATA_MANAGER_OK;
}

/** Add new file type entry to the file type table and allocate a region of
 *  memory to store file data within
 *
 * @param type FileType_t object representing file type definition to be
 *             stored into persistent storage medium
 * @param quantity_to_store Number of unique entries of this file type to be stored
 * @return Indicates success or failure reason
 */
int DataManager::add_file_type(DataManager_FileSystem::FileType_t type, uint16_t quantity_to_store)
{
    int requested_space = quantity_to_store * type.parameters.length_bytes;

    DataManager_FileSystem::GlobalStats_t g_stats;

    int g_stats_status = get_global_stats(g_stats.data);

    if(g_stats_status != DataManager::DATA_MANAGER_OK)
    {
        return g_stats_status;
    }

    if(requested_space > g_stats.parameters.space_remaining)
    {
        return DataManager::FILE_TYPE_INSUFFICIENT_SPACE;
    }

    type.parameters.file_start_address = g_stats.parameters.next_available_address;
    type.parameters.next_available_address = g_stats.parameters.next_available_address;
    type.parameters.file_end_address = (g_stats.parameters.next_available_address + requested_space) - 1;

    g_stats.parameters.next_available_address = type.parameters.file_end_address + 1;
    g_stats.parameters.space_remaining = EEPROM_SIZE_BYTES - g_stats.parameters.next_available_address; 

    g_stats_status = set_global_stats(g_stats.data);

    if(g_stats_status != DataManager::DATA_MANAGER_OK)
    {
        return g_stats_status;
    }

    type.parameters.valid = type.parameters.type_id + type.parameters.length_bytes + type.parameters.file_start_address +
                            type.parameters.file_end_address + type.parameters.next_available_address;

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
                               record.parameters.type_id);

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
    uint32_t checksum = (type.parameters.type_id + type.parameters.length_bytes + type.parameters.file_start_address +
                         type.parameters.file_end_address + type.parameters.next_available_address) & 0x000000FF;

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
                         record.parameters.type_id) & 0x000000FF;

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

/** Calculate number of measurements that can be stored
 *
 * @param type_id ID of the file to be queried
 * @param &remaining_entries Address of integer value to which the number
 *                           of remaining measurements should be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_remaining_file_entries(uint8_t type_id, int &remaining_entries)
{
    DataManager_FileSystem::FileType_t type;

    int status = get_file_type_by_id(type_id, type);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    int remaining_length = (type.parameters.file_end_address + 1) 
                          - type.parameters.next_available_address;

    remaining_entries = remaining_length / type.parameters.length_bytes;

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate remaining space for measurement in bytes
 *
 * @param type_id ID of the file to be queried
 * @param &remaining_entries Address of integer value to which the amount
 *                           of remaining space should be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_remaining_file_size(uint8_t type_id, int &remaining_bytes)
{
    DataManager_FileSystem::FileType_t type;

    int status = get_file_type_by_id(type_id, type);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    remaining_bytes = (type.parameters.file_end_address + 1) 
                     - type.parameters.next_available_address;

    return DataManager::DATA_MANAGER_OK;
}

/** Write actual data, i.e. a measurement, to next available address
 *  within the files allocated memory region
 *
 * @param type_id ID of the file to which we should append data
 * @param *data Actual data to be written to file
 * @param data_length Length of *data in bytes
 * @return Indicates success or failure reason
 */
int DataManager::append_to_file(uint8_t type_id, char *data, int data_length)
{
    DataManager_FileSystem::FileType_t type;

    int status = get_file_type_by_id(type_id, type);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    if(data_length != type.parameters.length_bytes)
    {
        return DataManager::FILE_TYPE_LENGTH_MISMATCH;
    }

    if((data_length - 1) + type.parameters.next_available_address 
       > type.parameters.file_end_address)
    {
        return DataManager::FILE_CONTENTS_INSUFFICIENT_SPACE;
    }

    /** Write actual data, i.e. a measurement, to the next available address 
     */
    status = _storage.write_to_address(type.parameters.next_available_address, 
                                       data, data_length);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    type.parameters.next_available_address += data_length;
    type.parameters.valid = type.parameters.type_id + 
                            type.parameters.length_bytes + 
                            type.parameters.file_start_address +
                            type.parameters.file_end_address + 
                            type.parameters.next_available_address;
    
    /** Update the next available address and validity byte
     */
    status = modify_file_type(type_id, type);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** By resetting the next available address to the file start address
 *  we essentially 'delete' all contents of the file while retaining the
 *  actual data until it is overwritten
 *
 * @param type_id ID of the file type definition whose contents are to be cleared
 * @return Indicates success or failure reason
 */  
int DataManager::delete_file_contents(uint8_t type_id)
{
    DataManager_FileSystem::FileType_t type;

    int status = get_file_type_by_id(type_id, type);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    type.parameters.next_available_address = type.parameters.file_start_address;
    type.parameters.valid = type.parameters.type_id + 
                            type.parameters.length_bytes + 
                            type.parameters.file_start_address +
                            type.parameters.file_end_address + 
                            type.parameters.next_available_address;

    status = modify_file_type(type_id, type);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

int DataManager::overwrite_file(uint8_t type_id, char *data)
{
    return 0;
}

int DataManager::truncate_file(uint8_t type_id, int entries_to_truncate)
{
    return 0;
}

/** Modify file type definition
 *
 * @param type_id ID of file type definition to be modified
 * @param type Updated version of file type definition
 * @return Indicates success or failure reason
 */
int DataManager::modify_file_type(uint8_t type_id, DataManager_FileSystem::FileType_t type)
{
    uint16_t max_types = get_max_types();
    int type_size = sizeof(type);
    
    bool match = false;
    uint16_t address;

    DataManager_FileSystem::FileType_t read_type;

    for(uint16_t type_index = 0; type_index < max_types; type_index++)
    {
        address = TYPE_STORE_START_ADDRESS + (type_index * type_size);
        int status = _storage.read_from_address(address, read_type.data, type_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file_type(read_type))
        {
            continue;
        }

        if(type_id == read_type.parameters.type_id)
        {
            match = true;
            break;
        }
    }

    if(!match)
    {
        return DataManager::DATA_MANAGER_INVALID_TYPE;
    }

    int status = _storage.write_to_address(address, type.data, type_size);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }
    
    return DataManager::DATA_MANAGER_OK;
}

