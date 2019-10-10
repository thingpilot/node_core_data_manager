/**
  * @file    DataManager.cpp
  * @version 0.1.0
  * @author  Rafaella Neofytou, Adam Mitchell
  * @brief   C++ file of the DataManager. Provides a very lightweight filesystem to facilitate the
  *          storage of arbitrary file types
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

/** Initialise the file table to all zeros, set file system initialised flag 
 *  and set g_stats next available address and space remaining 
 *
 * @return Indicates success or failure reason
 */
int DataManager::init_filesystem()
{
    char blank[PAGE_SIZE_BYTES] = { 0 };

    int status;

    for(int ft_page = 0; ft_page < FILE_TABLE_PAGES; ft_page++)
    {
        status = _storage.write_to_address(FILE_TABLE_START_ADDRESS + (ft_page * PAGE_SIZE_BYTES), blank, PAGE_SIZE_BYTES);

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
    g_stats.parameters.initialised = DataManager_FileSystem::INITIALISED; 

    status = set_global_stats(g_stats.data);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Determine whether or not the filesystem has been initialised
 *
 * @param &initialised Address of boolean value to which result of 
 *                     an initialisation check is stored. True on 
 *                     initialised, else false
 * @return Indicates success or failure reason
 */
int DataManager::is_initialised(bool &initialised)
{
    DataManager_FileSystem::GlobalStats_t g_stats;
    
    int status = get_global_stats(g_stats.data);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    if(g_stats.parameters.initialised != DataManager_FileSystem::INITIALISED)
    {
        initialised = false;
    }

    initialised = true;

    return DataManager::DATA_MANAGER_OK;
}

/** Get global next address and space remaining counters
 *
 * @param *data Byte array to which to write global stats counters
 * @return Indicates success or failure reason
 */
int DataManager::get_global_stats(char *data)
{
    int status = _storage.read_from_address(GLOBAL_STATS_START_ADDRESS, data, GLOBAL_STATS_LENGTH);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Return maximum number of files that can be stored
 *  in persistent storage
 *
 *  @return Maximum number of files that can be stored
 */
uint16_t DataManager::get_max_files()
{
	return (uint16_t)FILE_TABLE_LENGTH / sizeof(DataManager_FileSystem::File_t);
}

/** Return overall total file entry storage size in bytes
 *
 *  @return Total usable space, in bytes, for file entry storage
 */
int DataManager::get_storage_size_bytes()
{
	return (int)STORAGE_LENGTH;
}

/** Add new file to the file table and allocate a region of
 *  memory within which to store entries to the file
 *
 * @param file File_t object representing the file to be stored
 * @param entries_to_store Number of unique entries of this file type to be stored
 * @return Indicates success or failure reason
 */
int DataManager::add_file(DataManager_FileSystem::File_t file, uint16_t entries_to_store)
{
    int requested_space = entries_to_store * file.parameters.length_bytes;

    DataManager_FileSystem::GlobalStats_t g_stats;

    int g_stats_status = get_global_stats(g_stats.data);

    if(g_stats_status != DataManager::DATA_MANAGER_OK)
    {
        return g_stats_status;
    }

    if(requested_space > g_stats.parameters.space_remaining)
    {
        return DataManager_FileSystem::FILE_TABLE_FULL;
    }

    file.parameters.file_start_address = g_stats.parameters.next_available_address;
    file.parameters.next_available_address = g_stats.parameters.next_available_address;
    file.parameters.file_end_address = (g_stats.parameters.next_available_address + requested_space) - 1;

    g_stats.parameters.next_available_address = file.parameters.file_end_address + 1;
    g_stats.parameters.space_remaining = EEPROM_SIZE_BYTES - g_stats.parameters.next_available_address; 

    g_stats_status = set_global_stats(g_stats.data);

    if(g_stats_status != DataManager::DATA_MANAGER_OK)
    {
        return g_stats_status;
    }

    file.parameters.valid = file.parameters.filename + file.parameters.length_bytes + file.parameters.file_start_address +
                            file.parameters.file_end_address + file.parameters.next_available_address;

    int address = -1;
    int next_address_status = get_next_available_file_table_address(address);

    if(next_address_status != DataManager::DATA_MANAGER_OK) 
    {
        return next_address_status;
    }

    if(address == -1)
    {
        return DataManager_FileSystem::FILE_TABLE_FULL;
    }

    int write_status = _storage.write_to_address(address, file.data, sizeof(file));

    if(write_status != DataManager::DATA_MANAGER_OK)
    {
        return write_status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Get all File_t parameters for a given filename
 *
 * @param filename ID of file to be retrieved
 * @param &file Address of File_t object in which retrieved information
 *              will be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_file_by_name(uint8_t filename, DataManager_FileSystem::File_t &file)
{
    int file_size = sizeof(DataManager_FileSystem::File_t);

    uint16_t max_files = get_max_files();
    bool match = false;

    for(uint16_t file_index = 0; file_index < max_files; file_index++)
    {
        int status = _storage.read_from_address(FILE_TABLE_START_ADDRESS + (file_index * file_size), file.data, file_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file(file))
        {
            continue;
        }

        if(filename == file.parameters.filename)
        {
            match = true;
            break;
        }
    }

    if(!match)
    {
        return DataManager_FileSystem::FILE_INVALID_NAME;
    }
    
    return DataManager::DATA_MANAGER_OK;
}

/** Calculate the number of valid files current stored in memory
 * @param &valid_files   Address of integer value in which number of 
 *                       detected valid files will be stored
 * @return Indicates success or failure reason                        
 */
int DataManager::total_stored_files(int &valid_files)
{
    DataManager_FileSystem::File_t file;
    int file_size = sizeof(file);
    
    uint16_t max_files = get_max_files();

    for(uint16_t file_index = 0; file_index < max_files; file_index++)
    {
        int status = _storage.read_from_address(FILE_TABLE_START_ADDRESS + (file_index * file_size), file.data, file_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(is_valid_file(file))
        {
            valid_files++;
        }
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate total number of spaces available in the file table
 *  for new entries
 *
 * @param &remaining_files Address of integer value in which the total number
 *                         of spaces available in the file table is to be
 *                         written
 * @return Indicates success or failure reason
 */
int DataManager::total_remaining_file_table_entries(int &remaining_files)
{
    int valid_files = 0;
    int status = total_stored_files(valid_files);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status; 
    }

    uint16_t max_files = get_max_files();

    remaining_files = max_files - valid_files;

    return DataManager::DATA_MANAGER_OK;
}

/** Read an entry, i.e. actual data such as a measurement, from a 
 *  specific index within a file
 *
 * @param filename ID of the file from which we should read
 * @param entry_index 0-indexed position of the entry to be read
 * @param *data Pointer to an array in which the read data will be stored
 * @param data_length Length of *data in bytes
 * @return Indicates success or failure reason
 */
int DataManager::read_file_entry(uint8_t filename, int entry_index, char *data, int data_length)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    int total_written_entries = 0;
    status = get_total_written_file_entries(filename, total_written_entries);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    if(entry_index + 1 > total_written_entries)
    {
        return DataManager_FileSystem::FILE_ENTRY_INVALID_INDEX;
    }

    if(data_length != file.parameters.length_bytes)
    {
        return DataManager_FileSystem::FILE_ENTRY_LENGTH_MISMATCH;
    }

    uint16_t address = file.parameters.file_start_address + (entry_index * data_length);
    status = _storage.read_from_address(address, data, data_length);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Write an entry, i.e. actual data such as a measurement, to next 
 *  available address within the files allocated memory region
 *
 * @param filename ID of the file to which we should append data
 * @param *data Actual data to be written to file
 * @param data_length Length of *data in bytes
 * @return Indicates success or failure reason
 */
int DataManager::append_file_entry(uint8_t filename, char *data, int data_length)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    if(data_length != file.parameters.length_bytes)
    {
        return DataManager_FileSystem::FILE_ENTRY_LENGTH_MISMATCH;
    }

    if((data_length - 1) + file.parameters.next_available_address 
       > file.parameters.file_end_address)
    {
        return DataManager_FileSystem::FILE_ENTRY_FULL;
    }

    /** Write actual data, i.e. a measurement, to the next available address 
     */
    status = _storage.write_to_address(file.parameters.next_available_address, 
                                       data, data_length);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    file.parameters.next_available_address += data_length;
    file.parameters.valid = file.parameters.filename + 
                            file.parameters.length_bytes + 
                            file.parameters.file_start_address +
                            file.parameters.file_end_address + 
                            file.parameters.next_available_address;
    
    /** Update the next available address and validity byte
     */
    status = modify_file(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** By resetting the next available address to the file start address
 *  we essentially 'delete' all entries within the file whilst 
 *  retaining the actual data until it is overwritten
 *
 * @param filename ID of the file whose entries are to be cleared
 * @return Indicates success or failure reason
 */  
int DataManager::delete_file_entries(uint8_t filename)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    file.parameters.next_available_address = file.parameters.file_start_address;
    file.parameters.valid = file.parameters.filename + 
                            file.parameters.length_bytes + 
                            file.parameters.file_start_address +
                            file.parameters.file_end_address + 
                            file.parameters.next_available_address;

    status = modify_file(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Write an entry, i.e. actual data such as a measurement, to the 
 *  first address within the files allocated memory region
 *
 * @param filename ID of the file to which we should write data
 * @param *data Actual data to be written to file
 * @param data_length Length of *data in bytes
 * @return Indicates success or failure reason
 */
int DataManager::overwrite_file_entries(uint8_t filename, char *data, int data_length)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    if(data_length != file.parameters.length_bytes)
    {
        return DataManager_FileSystem::FILE_ENTRY_LENGTH_MISMATCH;
    }

    /** Write actual data, i.e. a measurement, to the start address 
     */
    status = _storage.write_to_address(file.parameters.file_start_address, 
                                       data, data_length);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    file.parameters.next_available_address = file.parameters.file_start_address + data_length;
    file.parameters.valid = file.parameters.filename + 
                            file.parameters.length_bytes + 
                            file.parameters.file_start_address +
                            file.parameters.file_end_address + 
                            file.parameters.next_available_address;
    
    /** Update the next available address and validity byte
     */
    status = modify_file(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Remove entries_to_remove entries starting from index 0, shift
 *  the remaining entries to the start of the file entry table and 
 *  set the next available address to the lowest available address. 
 *  This frees up space at the end of the file entry table by removing
 *  the most historic data
 *
 * @param filename ID of the file on which this operation is to be performed
 * @param entries_to_remove Number of entries to be truncated from the 
 *                          start of the file entry table
 * @return Indicates success or failure reason
 */
int DataManager::truncate_file(uint8_t filename, int entries_to_remove)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    int written_entries = 0;
    status = get_total_written_file_entries(filename, written_entries);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    if(entries_to_remove >= written_entries)
    {
        status = delete_file_entries(filename);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        return DataManager::DATA_MANAGER_OK;
    }

    char buffer[file.parameters.length_bytes];
    int new_index = 0;

    for(int current_index = entries_to_remove; current_index < written_entries; current_index++)
    {
        status = read_file_entry(filename, current_index, buffer, file.parameters.length_bytes);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        uint16_t new_address = file.parameters.file_start_address + (new_index * file.parameters.length_bytes);

        status = _storage.write_to_address(new_address, buffer, file.parameters.length_bytes);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        new_index++;
    }

    file.parameters.next_available_address = file.parameters.file_start_address + (new_index * file.parameters.length_bytes); 
    file.parameters.valid = file.parameters.filename + 
                            file.parameters.length_bytes + 
                            file.parameters.file_start_address +
                            file.parameters.file_end_address + 
                            file.parameters.next_available_address;
    
    /** Update the next available address and validity byte
     */
    status = modify_file(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate number of entries within a file
 *
 * @param filename ID of the file to be queried
 * @param &written_entries Address of integer value to which the number
 *                         of written entries should be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_total_written_file_entries(uint8_t filename, int &written_entries)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    int remaining_length = (file.parameters.file_end_address + 1) 
                          - file.parameters.next_available_address;

    int remaining_entries = remaining_length / file.parameters.length_bytes;

    int total_entries = ((file.parameters.file_end_address - file.parameters.file_start_address) + 1 ) 
                        / file.parameters.length_bytes;

    written_entries = total_entries - remaining_entries;

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate number of measurements that can be stored
 *
 * @param filename ID of the file to be queried
 * @param &remaining_entries Address of integer value to which the number
 *                           of remaining measurements should be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_remaining_file_entries(uint8_t filename, int &remaining_entries)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    int remaining_length = (file.parameters.file_end_address + 1) 
                          - file.parameters.next_available_address;

    remaining_entries = remaining_length / file.parameters.length_bytes;

    return DataManager::DATA_MANAGER_OK;
}

/** Calculate remaining space for entries in bytes
 *
 * @param filename ID of the file to be queried
 * @param &remaining_entries Address of integer value to which the amount
 *                           of remaining space should be stored
 * @return Indicates success or failure reason
 */
int DataManager::get_remaining_file_entries_bytes(uint8_t filename, int &remaining_bytes)
{
    DataManager_FileSystem::File_t file;

    int status = get_file_by_name(filename, file);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    remaining_bytes = (file.parameters.file_end_address + 1) 
                     - file.parameters.next_available_address;

    return DataManager::DATA_MANAGER_OK;
}

/** Set global next address and space remaining counters
 *
 * @param data Byte array containing data to write to global stats counters
 * @return Indicates success or failure reason
 */
int DataManager::set_global_stats(char *data)
{
    int status = _storage.write_to_address(GLOBAL_STATS_START_ADDRESS, data, GLOBAL_STATS_LENGTH);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Perform checksum on given File_t using the 'valid' parameter
 *
 * @param type File to be checked for validity
 * @return True if file is valid, else false
 */
bool DataManager::is_valid_file(DataManager_FileSystem::File_t file)
{
    /** During init_filesystem() we set every bit in the file table to 0
     *  so, if the valid byte == 0, this can't be a valid entry
     */
    if(file.parameters.valid == 0x00)
    {
        return false;
    }
    
    /** Mask the first 24 bits so that we can use our 8-bit valid flag as a rudimentary checksum 
     *  of the length and type id
     */
    uint32_t checksum = (file.parameters.filename + file.parameters.length_bytes + file.parameters.file_start_address +
                         file.parameters.file_end_address + file.parameters.next_available_address) & 0x000000FF;

    if(file.parameters.valid != checksum)
    {
        return false;
    }

    return true;
}

/** Determine the next available address to which to write file
 *
 * @param &next_available_address Address of integer value in which the address
 *                                of the next available location in memory to which
 *                                you can write a file is stored. -1 if 
 *                                there are no available spaces
 * @return Indicates success or failure reason
 */
int DataManager::get_next_available_file_table_address(int &next_available_address)
{
    DataManager_FileSystem::File_t file;
    int file_size = sizeof(file);
    
    uint16_t max_files = get_max_files();

    for(uint16_t file_index = 0; file_index < max_files; file_index++)
    {
        int address = FILE_TABLE_START_ADDRESS + (file_index * file_size);
        int status = _storage.read_from_address(address, file.data, file_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file(file))
        {
            next_available_address = address;
            break;
        }
    }

    return DataManager::DATA_MANAGER_OK;
}

/** Modify a file's metadata
 *
 * @param filename ID of file to be modified
 * @param file Updated version of file
 * @return Indicates success or failure reason
 */
int DataManager::modify_file(uint8_t filename, DataManager_FileSystem::File_t file)
{
    uint16_t max_files = get_max_files();
    int file_size = sizeof(file);
    
    bool match = false;
    uint16_t address;

    DataManager_FileSystem::File_t read_file;

    for(uint16_t file_index = 0; file_index < max_files; file_index++)
    {
        address = FILE_TABLE_START_ADDRESS + (file_index * file_size);
        int status = _storage.read_from_address(address, read_file.data, file_size);

        if(status != DataManager::DATA_MANAGER_OK)
        {
            return status;
        }

        if(!is_valid_file(read_file))
        {
            continue;
        }

        if(filename == read_file.parameters.filename)
        {
            match = true;
            break;
        }
    }

    if(!match)
    {
        return DataManager_FileSystem::FILE_INVALID_NAME;
    }

    int status = _storage.write_to_address(address, file.data, file_size);

    if(status != DataManager::DATA_MANAGER_OK)
    {
        return status;
    }
    
    return DataManager::DATA_MANAGER_OK;
}

#if defined (DM_DBG) && (DM_DBG == true)
/** Utility function to print a File_t over UART
 *
 * &pc Serial object over which to print the File_t object parameters
 * file The File_t object whose parameters we wish to print
 */
void DataManager::print_file(Serial &pc, DataManager_FileSystem::File_t file)
{
    pc.printf("---PRINT FILE---\r\n");
    pc.printf("Filename: %u\r\n", file.parameters.filename);
    pc.printf("Length_bytes: %u\r\n", file.parameters.length_bytes);
    pc.printf("File_start_address: %u\r\n", file.parameters.file_start_address);
    pc.printf("File_end_address: %u\r\n", file.parameters.file_end_address);
    pc.printf("Next_available_address: %u\r\n", file.parameters.next_available_address);
    pc.printf("Valid: %u\r\n", file.parameters.valid);

    int written_entries = 0;
    get_total_written_file_entries(file.parameters.filename, written_entries);
    pc.printf("Written_entries: %i\r\n", written_entries);

    int remaining_entries = 0;
    get_remaining_file_entries(file.parameters.filename, remaining_entries);
    pc.printf("Remaining_entries: %i\r\n", remaining_entries);

    int remaining_entries_bytes = 0;
    get_remaining_file_entries_bytes(file.parameters.filename, remaining_entries_bytes);
    pc.printf("Remaining_entries_bytes: %i\r\n", remaining_entries_bytes);
    pc.printf("---END PRINT FILE---\r\n");
}

/** Utility function to print GlobalStats_t over UART
 *
 * &pc Serial object over which to print the GlobalStats_t object parameters
 * g_stats The GlobalStats_t object whose parameters we wish to print
 */
void DataManager::print_global_stats(Serial &pc, DataManager_FileSystem::GlobalStats_t g_stats)
{
    pc.printf("---PRINT GLOBAL STATS\r\n");
    pc.printf("Space_remaining_bytes: %u\r\n", g_stats.parameters.space_remaining);
    pc.printf("Next_available_address: %u\r\n", g_stats.parameters.next_available_address);
    pc.printf("---END PRINT GLOBAL STATS\r\n");
}
#endif // #if defined (DM_DBG) && (DM_DBG == true)

