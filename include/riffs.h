#ifndef __riffs_h
#define __riffs_h

#include <utility/list.h>
#include <utility/string.h>
#include <persistent_storage.h>

__BEGIN_SYS

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef int int32_t;
typedef short int16_t;

const uint16_t MAGIC_NUMBER = 0xAAAA;
const uint16_t CURRENT_VERSION = 0x0001;

uint32_t file_counter_id = 1;

class RIFFS_SectorHeader {
public:
    volatile uint32_t _magic_number;
    volatile uint32_t _erased_times;
    volatile uint32_t _sector_size;

    RIFFS_SectorHeader(){
        _magic_number = 0;
        _erased_times = 0;
        _sector_size = 0;
    }
};

class RIFFS_FlashDataControl {
public:
    enum Type {
        LOG_CONTEXT = 0xAA0F,
        ENTRY_CONTEXT = 0x5500,
        DATA = 0x55F0,
        SECTOR = 0xAAAA
    };

    enum Data_Status {
        TEMPORARY_WRITING = 0x7FFF,
        TEMPORARY_DELETE = 0x7000,
        DELETED = 0x0000
    };
public:
    uint32_t _data_offset;
    uint32_t _data_size;
    uint32_t _data_id;
    uint16_t _data_type;
    uint16_t _data_version;

    RIFFS_FlashDataControl(){
        _data_offset = 0;
        _data_size = 0;
        _data_id = 0;
        _data_type = 0;
        _data_version = 0;
    }
};
typedef Simple_List<RIFFS_FlashDataControl>::Element Flash_Element;

class RIFFS_MemoryDataControl : public RIFFS_FlashDataControl {
public:
    RIFFS_MemoryDataControl() : RIFFS_FlashDataControl() {
        _sector_id = 0xFFFFFFFF;
        _control_offset = 0xFFFFFFFF;
    }

    uint32_t _sector_id;
    uint32_t _control_offset;
};
typedef Simple_Ordered_List<RIFFS_MemoryDataControl>::Element Memory_Element;

class RIFFS_Common {
public:
    /*
    * It reads the header on the specified address
    * Receives the persistent storage device
    * Receives the header object to be red
    * Receives the address where the sector is located
    **
    *    -> returns true for a valid header
    *    -> returns false for a invalid header
    */
    static bool get_header(Persistent_Storage *device, RIFFS_SectorHeader *header, const uint32_t address){
        device->read(address, header, sizeof(RIFFS_SectorHeader));

        return (header->_magic_number == MAGIC_NUMBER)?true:false;
    }

    /*
    * It checks if chunk of data can be data control
    * TODO: make it more precise, adding more validation
    */
    static bool is_data_control(const RIFFS_FlashDataControl *data_control) {
        if(
            data_control->_data_type == RIFFS_FlashDataControl::LOG_CONTEXT ||
            data_control->_data_type == RIFFS_FlashDataControl::ENTRY_CONTEXT ||
            data_control->_data_type == RIFFS_FlashDataControl::DATA ||
            data_control->_data_type == RIFFS_FlashDataControl::SECTOR
          ) {

            return true;

        }
        return false;
    }

    /*
        Some flash memories have to be written in blocks of data
        This method fix buffers that are not equivalent to the flash block
    */
    static bool linearize_char(char *buffer, int *str_size, int original_size) {
        int size = 0;

        while(size <= original_size || buffer[size] != '\0') {
            size++;
        }

        int diff = Persistent_Storage::word_size() - (size % Persistent_Storage::word_size());
        if(diff != 0) {
            db<Persistent_Storage>(TRC) << "Data needs to be linearized" << endl;
            db<Persistent_Storage>(TRC) << "    * Size is: " << size << endl;
            db<Persistent_Storage>(TRC) << "    * Bytes added: " << diff << endl;

            char temp[size + diff];
            memset(temp, 0, size + diff);

            strncpy(temp, buffer, size + diff);

            buffer = temp;

            *str_size = size + diff;
        }
    }
};

class RIFFS_Scanner {
private:
    Persistent_Storage *_device;

    void append_to_list(const RIFFS_FlashDataControl * data_control, uint32_t control_offset, uint32_t sector_id){

        if(data_control->_data_type == RIFFS_FlashDataControl::SECTOR) {
            Flash_Element *e = new Flash_Element(data_control);
            _sector_list.insert(e);

            db<Persistent_Storage>(TRC) << "Data id [" << _sector_list.tail()->object()->_data_id << "] was added to the list" << endl;
            db<Persistent_Storage>(TRC) << "Its size is: " << _sector_list.tail()->object()->_data_size << endl;
            db<Persistent_Storage>(TRC) << "Offset is: " << _sector_list.tail()->object()->_data_offset << endl;
        } else {
            RIFFS_MemoryDataControl *mem = new RIFFS_MemoryDataControl;
            mem->_control_offset = control_offset;
            mem->_sector_id = sector_id;
            mem->_data_offset = data_control->_data_offset;
            mem->_data_size = data_control->_data_size;
            mem->_data_id = data_control->_data_id;
            mem->_data_type = data_control->_data_type;
            mem->_data_version = data_control->_data_version;

            Simple_List<RIFFS_MemoryDataControl>::Element *e = new Simple_List<RIFFS_MemoryDataControl>::Element(mem);
            _data_list.insert(e);

            db<Persistent_Storage>(TRC) << "Data id [" << _data_list.tail()->object()->_data_id << "] was added to the list" << endl;
            db<Persistent_Storage>(TRC) << "Its size is: " << _data_list.tail()->object()->_data_size << endl;
            db<Persistent_Storage>(TRC) << "Offset is: " << _data_list.tail()->object()->_data_offset << endl;
        }
    }

    /*
        * Goes through the end of every sector searching for data control
        * All data control found is put into a list if flag is true

        * returns the amount of data control found
    */
    uint32_t scan_sector(uint32_t address, uint32_t size, int32_t sector_id, bool add_on_list){

        address = address + size - sizeof(RIFFS_FlashDataControl);

        bool keepReading = true;
        int count = 0;

        while(keepReading) {
            RIFFS_FlashDataControl *data_control = new RIFFS_FlashDataControl;

            _device->read(address, data_control, sizeof(RIFFS_FlashDataControl));

            if(RIFFS_Common::is_data_control(data_control)) {
                if(add_on_list)
                    append_to_list(data_control, address, sector_id);

                count++;

                address -= sizeof(RIFFS_FlashDataControl);
            } else {
                keepReading = false;
            }
        }

        return count;
    }

    int sector_available_size(uint32_t address, uint32_t size, int32_t sector_id, uint32_t *count){

        address = size - sizeof(RIFFS_FlashDataControl);

        bool keepReading = true;
        *count = 0;

        int base_address = size*sector_id;

        int available = size;

        while(keepReading) {
            RIFFS_FlashDataControl *data_control = new RIFFS_FlashDataControl;

            _device->read(base_address + address, data_control, sizeof(RIFFS_FlashDataControl));

            if(RIFFS_Common::is_data_control(data_control)) {
                (*count)++;

                available -= data_control->_data_size;
                available -= sizeof(RIFFS_FlashDataControl);

                address -= sizeof(RIFFS_FlashDataControl);
            } else {
                keepReading = false;
                address += sizeof(RIFFS_FlashDataControl);

                return available - sizeof(RIFFS_FlashDataControl);
            }
        }

        return *count;
    }

    int sector_available_size(uint32_t address, uint32_t size, int32_t sector_id, uint32_t *count, uint32_t *offset){

        address = size - sizeof(RIFFS_FlashDataControl);

        bool keepReading = true;
        *count = 0;

        *offset = 0;

        int base_address = size*sector_id;

        int available = size;

        while(keepReading) {
            RIFFS_FlashDataControl *data_control = new RIFFS_FlashDataControl;

            _device->read(base_address + address, data_control, sizeof(RIFFS_FlashDataControl));

            if(RIFFS_Common::is_data_control(data_control)) {
                (*count)++;

                *offset = *offset + data_control->_data_size;

                available -= data_control->_data_size;
                available -= sizeof(RIFFS_FlashDataControl);

                address -= sizeof(RIFFS_FlashDataControl);

                delete(data_control);

            } else {
                keepReading = false;

                address += sizeof(RIFFS_FlashDataControl);

                delete(data_control);

                return available - sizeof(RIFFS_FlashDataControl);
            }
        }


        return *count;
    }
public:
    RIFFS_Scanner(Persistent_Storage *device) {
        _device = device;
    }

    Simple_List<RIFFS_FlashDataControl> _sector_list;
    Simple_List<RIFFS_MemoryDataControl> _data_list;

    /*
        * Goes through every sector
        * Calls for a deeper search for each sector
    */
    void scan(){
        db<Persistent_Storage>(TRC) << "RIFFS_Scanner::scan()" << endl;

        uint32_t sector_address;
        uint32_t sector_size;

        for(int i = 0; i < _device->pages(); i++){

            _device->get_sector_address(i, &sector_address, &sector_size);
            db<Persistent_Storage>(TRC) << "Starting to scan sector [" << (i + 1) << "] at address: " << sector_address << endl;

            //if is not a valid sector, continues
            RIFFS_SectorHeader aux;
            if(!RIFFS_Common::get_header(_device, &aux, sector_address)) {
                db<Persistent_Storage>(TRC) << "Not a valid sector, aborting..........." << endl << endl;
                continue;
            }

            scan_sector(sector_address, sector_size, i, true);

            db<Persistent_Storage>(TRC) << "-------------------" << endl << endl;
        }

        db<Persistent_Storage>(TRC) << endl << endl << "**************************************************";
        db<Persistent_Storage>(TRC) << endl << "Flash System Scan done. Info is: " << endl;
        db<Persistent_Storage>(TRC) << "    * Sectors Found: " << _sector_list.size() << endl;
        db<Persistent_Storage>(TRC) << "    * Data Found: " << _data_list.size() << endl;
        db<Persistent_Storage>(TRC) << "**************************************************" << endl << endl;
    }

    /*
        * Checks if the sector is empty
        * Returns true for empty
    */
    int available_space(uint32_t sector_index, uint32_t *count) {
        uint32_t address;
        uint32_t size;

        _device->get_sector_address(sector_index, &address, &size);

        return sector_available_size(address, size, sector_index, count);
    }

    int available_space(uint32_t sector_index, uint32_t *count, uint32_t *offset) {
        uint32_t address;
        uint32_t size;

        _device->get_sector_address(sector_index, &address, &size);

        return sector_available_size(address, size, sector_index, count, offset);
    }

    RIFFS_SectorHeader get_header_info(uint32_t address, uint32_t size) {
        RIFFS_SectorHeader header;
        _device->read(address, &header, size);
        return header;
    }

    uint32_t data_list_count(){
        return _data_list.size();
    }
};

class RIFFS_Allocator {
private:
    static RIFFS_Scanner *_scanner;
    static Simple_Ordered_List<RIFFS_MemoryDataControl> _clean_blocks;
public:
    RIFFS_Allocator(RIFFS_Scanner *scanner){
        _scanner = scanner;

        init();
    }

    void init(){
        db<Persistent_Storage>(TRC) << "Begin allocation for empty sectors." << endl;
        db<Persistent_Storage>(TRC) << "There are [" << _scanner->_sector_list.size() << "] sectors to be inspected." << endl << endl;

        Flash_Element *aux = _scanner->_sector_list.head();
        uint32_t count = 0;
        uint32_t offset = 0;
        int available_space = 0;

        //for each valid sector checks if is empty
        for(int i = 0; i < _scanner->_sector_list.size(); i++){
            RIFFS_FlashDataControl *data_control = aux->object();

            db<Persistent_Storage>(TRC) << "Scanning sector: [" << (i+1) << "]" << endl;
            db<Persistent_Storage>(TRC) << "    * Data offset: " << data_control->_data_offset << endl;
            db<Persistent_Storage>(TRC) << "    * Data type: " << data_control->_data_type << endl;
            db<Persistent_Storage>(TRC) << "    * Data size: " << data_control->_data_size << endl;
            db<Persistent_Storage>(TRC) << "    * Data id: " << data_control->_data_id << endl;
            db<Persistent_Storage>(TRC) << "    * Data version: " << data_control->_data_version << endl;

            //if is empty it will add to the allocation list ordered by erased times
            count = 0;
            offset = 0;
            available_space = _scanner->available_space(i, &count, &offset);
            if(available_space > 0) { //sector is empty
                db<Persistent_Storage>(TRC) << "Sector [" << i << "] has (" << available_space << ") bytes available" << endl;
                db<Persistent_Storage>(TRC) << endl;

                RIFFS_MemoryDataControl *mem_control = new RIFFS_MemoryDataControl();
                mem_control->_data_offset = offset;
                mem_control->_control_offset = Persistent_Storage::page_size() - (1+count)*sizeof(RIFFS_FlashDataControl);
                mem_control->_sector_id = i;

                db<Persistent_Storage>(TRC) << "Memory Data control has following info: " << endl;
                db<Persistent_Storage>(TRC) << "    * Data Offset: " << mem_control->_data_offset << endl;
                db<Persistent_Storage>(TRC) << "    * Control Offset: " << mem_control->_control_offset << endl;
                db<Persistent_Storage>(TRC) << "    * Sector Id: " << mem_control->_sector_id << endl;
                db<Persistent_Storage>(TRC) << "    * Base Address is: " << Persistent_Storage::page_size()*i << endl;

                uint32_t element_rank = (_scanner->get_header_info(data_control->_data_offset, data_control->_data_size)._erased_times)*4*Persistent_Storage::page_size() - available_space;
                db<Persistent_Storage>(TRC) << "Adding sector to the list with rank: " << element_rank << endl;

                Memory_Element *el = new Memory_Element(mem_control, element_rank);
                _clean_blocks.insert(el);
            } else {
                db<Persistent_Storage>(TRC) << "  --> This sector has no free space" << endl;
            }

            db<Persistent_Storage>(TRC) << endl;

            aux = aux->next();
        }

        db<Persistent_Storage>(TRC) << endl << "**************************" << endl;
        db<Persistent_Storage>(TRC) << "Device has [" << _clean_blocks.size() << "] sectors ready" << endl;
        db<Persistent_Storage>(TRC) << "**************************" << endl;
    }

    bool alloc(RIFFS_MemoryDataControl *data_control) {
        db<Persistent_Storage>(TRC) << "RIFFS_Allocator::alloc(blocks size: " << dec << _clean_blocks.size() << ")" << endl;

        if(_clean_blocks.size() == 0)
            init();

        uint32_t size = data_control->_data_size;

        Memory_Element *element;
        RIFFS_MemoryDataControl *aux;

        uint32_t temp = 0;

        int blocks_size = _clean_blocks.size();

        for(int i = 0; i < blocks_size; i++){

            element = _clean_blocks.get(i);
            aux = element->object();

            db<Persistent_Storage>(TRC) << "----- Sector " << aux->_sector_id << " ------" << endl;
            db<Persistent_Storage>(TRC) << "   * Needs: " << size << endl;
            db<Persistent_Storage>(TRC) << "   * Has: " << _scanner->available_space(aux->_sector_id, &temp) << endl;
            db<Persistent_Storage>(TRC) << "   * Sector ID: " << aux->_sector_id << endl;

            if(size <= _scanner->available_space(aux->_sector_id, &temp)) {
                _clean_blocks.remove(element);

                data_control->_data_offset = aux->_data_offset;
                data_control->_sector_id = aux->_sector_id;
                data_control->_control_offset = aux->_control_offset;
                data_control->_data_id = file_counter_id;

                //delete(aux);

                return true;
            } else {
                db<Persistent_Storage>(TRC) << "Not enough space on sector " << i << endl;
            }
        }

        //delete(aux);

        return false;
    }
};
RIFFS_Scanner *RIFFS_Allocator::_scanner;
Simple_Ordered_List<RIFFS_MemoryDataControl> RIFFS_Allocator::_clean_blocks;

/***********************ENTRY BEGINS*************************/
class RIFFS_DataFile {
public:
    uint32_t _data_offset;
    uint32_t _control_offset;
    uint32_t _id;
    uint32_t _size;
    uint16_t _version;
public:
    RIFFS_DataFile () {}

    int init(uint32_t id, uint32_t data_offset, uint32_t control_offset, uint32_t size, uint16_t version){
        _data_offset = data_offset;
        _control_offset = control_offset;
        _id = id;
        _size = size;
        _version = version;
    }
};

class RIFFS_Entry {
public:
    enum EntryType {
        DIRECTORY = 0xAAAA,
        FILE = 0x5555
    };

    enum Offsets {
        FATHER_ID_OFFSET = 0x00,
        ENTRYTYPE_OFFSET = 0x04,
        FILENAME_OFFSET = 0x06
    };
public:
    uint32_t _father_id;
    uint16_t _type;
    char _name[256];
};

class RIFFS_ClassifiedEntry : public RIFFS_Entry {
public:
    uint16_t _version;
    RIFFS_DataFile *_data;
};

class RIFFS_EntryFile {
public:
    RIFFS_ClassifiedEntry *_entry;
    uint32_t _id;
};

class RIFFS_File : public RIFFS_EntryFile {
private:
    uint32_t _pointer;
public:
    Simple_List<RIFFS_DataFile> *_data_list;
    static RIFFS_Allocator *_allocator;
    static Persistent_Storage *_device;
public:
    RIFFS_File(uint32_t id){
        _id = id;
        _entry = 0;
        _pointer = 0;
        _data_list = new Simple_List<RIFFS_DataFile>;
    }

    RIFFS_File(uint32_t id, Persistent_Storage *device, RIFFS_Allocator *allocator){
        _id = id;
        _entry = 0;
        _data_list = new Simple_List<RIFFS_DataFile>;
        _pointer = 0;

        _allocator = allocator;
        _device = device;
    }

    uint32_t data_count() { return _data_list->size(); };

    static RIFFS_File *create_entry(RIFFS_Entry *entry, Persistent_Storage *device, RIFFS_Allocator *allocator, uint32_t name_size) {
        db<Persistent_Storage>(TRC) << "RIFFS_File::create_entry()" << endl;

        RIFFS_MemoryDataControl *data_control = new RIFFS_MemoryDataControl;
        RIFFS_File *file;
        int size = 0;

        size += name_size;

        size += sizeof(entry->_father_id);
        size += sizeof(entry->_type);

        db<Persistent_Storage>(TRC) << "Data control address is: " << data_control << endl;
        int diff = size % Persistent_Storage::word_size();
        data_control->_data_size = size + Persistent_Storage::word_size() - diff;

        bool free_space_found = allocator->alloc(data_control);

        if(!free_space_found) {
            db<Persistent_Storage>(TRC) << "There is no space to add the data" << endl;
            return 0;
        }

        db<Persistent_Storage>(TRC) << endl << "Allocated space for data_control" << endl;
        db<Persistent_Storage>(TRC) << "    * Data offset: " << data_control->_data_offset << endl;
        db<Persistent_Storage>(TRC) << "    * Data type: " << data_control->_data_type << endl;
        db<Persistent_Storage>(TRC) << "    * Data size: " << data_control->_data_size << endl;
        db<Persistent_Storage>(TRC) << "    * Data id: " << data_control->_data_id << endl;
        db<Persistent_Storage>(TRC) << "    * Data version: " << data_control->_data_version << endl;
        db<Persistent_Storage>(TRC) << "    * Control Offset: " << data_control->_control_offset << endl;
        db<Persistent_Storage>(TRC) << "    * Sector Id: " << data_control->_sector_id << endl << endl;

        data_control->_data_type = RIFFS_FlashDataControl::ENTRY_CONTEXT;

        data_control->_data_version = 0x01;
        data_control->_data_id = file_counter_id;
        file_counter_id += 1;

        file = new RIFFS_File(data_control->_data_id, device, allocator);
        RIFFS_DataFile *data_file = new RIFFS_DataFile();

        db<Persistent_Storage>(TRC) << "------------------------" << endl;
        file->write_data(data_control, (char *) entry, data_file);

        file->entry(data_file);

        return file;
    }

    void entry(RIFFS_DataFile *data_file) {
        db<Persistent_Storage>(TRC) << "RIFFS_FILE::entry()" << endl;
        char buffer[255];
        int buffer_size;

        if(_entry)
            delete(_entry);

        buffer_size = data_file->_size;
        _device->read(data_file->_data_offset, buffer, buffer_size);
        buffer[buffer_size] = '\0';

        _entry = new RIFFS_ClassifiedEntry;
        _entry->_father_id = *((uint32_t *) &buffer[RIFFS_Entry::FATHER_ID_OFFSET]);
        _entry->_type = *((uint16_t *) &buffer[RIFFS_Entry::ENTRYTYPE_OFFSET]);
        _entry->_version = data_file->_version;

        strcpy((char*) _entry->_name, (char*) &buffer[RIFFS_Entry::FILENAME_OFFSET]);
        _entry->_name[buffer_size] = '\0';

        _entry->_data = data_file;

        db<Persistent_Storage>(TRC) << "Entry info:" << endl;
        db<Persistent_Storage>(TRC) << "    * Father ID: " << _entry->_father_id << endl;
        db<Persistent_Storage>(TRC) << "    * Type: " << hex << _entry->_type << endl;
        db<Persistent_Storage>(TRC) << "    * Version: " << dec << _entry->_version << endl;
        db<Persistent_Storage>(TRC) << "    * Name: " << _entry->_name << endl;
    }

    int write_data(RIFFS_MemoryDataControl *data_control, char *buffer, RIFFS_DataFile *data_file) {
        uint32_t address;
        uint32_t size;

        db<Persistent_Storage>(TRC) << "Getting address for sector: " << dec << data_control->_sector_id << endl;
        _device->get_sector_address(data_control->_sector_id, &address, &size);

        return write_data(data_control, buffer, data_file, address);
    }

    int write_data(RIFFS_MemoryDataControl *data_control, char *buffer, RIFFS_DataFile *data_file, uint32_t base_address) {
        db<Persistent_Storage>(TRC) << "Will write:" << endl;
        db<Persistent_Storage>(TRC) << "    * Data[start=" << data_control->_data_offset  + base_address << ",size=" << data_control->_data_size << "]" << endl;
        db<Persistent_Storage>(TRC) << "    * Control[start=" << data_control->_control_offset + base_address << ",size=" << sizeof(RIFFS_FlashDataControl) << "]" << endl;
        db<Persistent_Storage>(TRC) << "    * Base is: " << base_address << endl;

        _device->write(data_control->_data_offset + base_address, buffer, data_control->_data_size);
        _device->write(data_control->_control_offset + base_address, (char *) data_control, sizeof(RIFFS_FlashDataControl));

        data_file->init(data_control->_data_id,
            data_control->_data_offset + base_address,
            data_control->_control_offset + base_address,
            data_control->_data_size,
            data_control->_data_version);

        return 1;
    }

    int write(char *buffer, uint32_t size) {
        db<Persistent_Storage>(TRC) << endl << endl << "RIFFS_File::write()" << endl;

        uint32_t max_version = 1;
        uint32_t base_address = 0;

        uint32_t total_size = this->size();
        
        db<Persistent_Storage>(TRC) << "Pointer is " << _pointer << ", total size: " << total_size << endl;

        if(_pointer > total_size) {
            
            return -1;

        } else if(_pointer == total_size) {
            
            return append(buffer, size);

        } else if (_pointer < total_size) {

            db<Persistent_Storage>(TRC) << "Write in the middle of the file" << endl;
            db<Persistent_Storage>(TRC) << "This file has " << _data_list->size() << " pieces." << endl;            
            
            RIFFS_MemoryDataControl *data_control = new RIFFS_MemoryDataControl;
            RIFFS_DataFile *data_file;

            int new_size = 0;
            RIFFS_Common::linearize_char(buffer, &new_size, size);
            
            db<Persistent_Storage>(TRC) << "This file has " << _data_list->size() << " pieces." << endl;

            int file_address = 0;
            int offset_address = 0;

            int temp_pointer = _pointer;

            for(int i = 0; i < _data_list->size(); i++){
                //PERCORRER a lista
                //Verificar se o ponteiro esta entre a posicao atual

                data_file = _data_list->get(i)->object();
                db<Persistent_Storage>(TRC) << endl << "Data file offset: " << data_file->_data_offset << ", size: " << data_file->_size << endl;
                db<Persistent_Storage>(TRC) << "Pointer is at " << _pointer << endl;
                db<Persistent_Storage>(TRC) << "File address: " << file_address << endl;
                //db<Persistent_Storage>(TRC) << "Offset + size: " << file_address << endl;

                if(_pointer <= file_address + data_file->_size && _pointer >= file_address) {
                    
                    db<Persistent_Storage>(TRC) << endl << "Pointer is here!" << endl;
                    db<Persistent_Storage>(TRC) << "address: " << data_file->_data_offset << endl;

                    offset_address = _pointer - file_address;
                    db<Persistent_Storage>(TRC) << "offset address: " << offset_address << endl;

                    if( data_file->_size - offset_address >= size ) {

                        db<Persistent_Storage>(TRC) << "we are able to update this data file" << endl;

                        db<Persistent_Storage>(TRC) << "Writing buffer at " << data_file->_data_offset + offset_address << endl;
                        db<Persistent_Storage>(TRC) << "Size to write on buffer is " << size  << endl;

                        _device->write(data_file->_data_offset + offset_address, buffer, size);
                    
                    } else {

                        db<Persistent_Storage>(TRC) << "not able to update everything on this data file" << endl;
                        db<Persistent_Storage>(TRC) << (_pointer + size) - data_file->_size << " bytes written here" << endl;

                        db<Persistent_Storage>(TRC) << "Writing buffer at " << data_file->_data_offset + _pointer << endl;
                        db<Persistent_Storage>(TRC) << "Size of buffer is " << (_pointer + size) - data_file->_size  << endl;
                        
                        _device->write(data_file->_data_offset + _pointer, buffer, (_pointer + size) - data_file->_size);
                        
                        size -= (_pointer + size) - data_file->_size;

                        _pointer = 0;

                        continue;
                    }

                    _pointer = temp_pointer;
                    break;
                
                } else {
                    file_address += data_file->_size;

                    db<Persistent_Storage>(TRC) << "Pointer is not here!" << endl;
                    db<Persistent_Storage>(TRC) << "new file_address: " << file_address << endl;
                }
                
            }
        }
    }

    int append(char *buffer, uint32_t size){
        db<Persistent_Storage>(TRC) << endl << endl << "RIFFS_File::append()" << endl;

        uint32_t max_version = 1;
        uint32_t base_address = 0;

        RIFFS_MemoryDataControl *data_control = new RIFFS_MemoryDataControl;
        RIFFS_DataFile *data_file;

        int new_size = 0;
        RIFFS_Common::linearize_char(buffer, &new_size, size);

        db<Persistent_Storage>(TRC) << "This file has " << _data_list->size() << " pieces." << endl;

        for(int i = 0; i < _data_list->size(); i++){
            data_file = _data_list->get(i)->object();
            if(data_file->_version >= max_version)
                max_version = data_file->_version + 1;
        }

        do {
            data_control->_data_size = size;

            db<Persistent_Storage>(TRC) << "Allocating space!" << endl;
            bool enough_space = RIFFS_File::_allocator->alloc(data_control);
            if(!enough_space) {
                db<Persistent_Storage>(TRC) << "Not enough space on the flash disk" << endl;
                return 0;
            }
            db<Persistent_Storage>(TRC) << "Allocation Done" << endl;

            size -= data_control->_data_size;

            data_control->_data_type = RIFFS_FlashDataControl::DATA;
            data_control->_data_version = max_version;
            max_version += 1;
            data_control->_data_id = this->_id;

            uint32_t temp;
            _device->get_sector_address(data_control->_sector_id, &base_address, &temp);

            db<Persistent_Storage>(TRC) << "Following will be appended to the file: " << endl;
            db<Persistent_Storage>(TRC) << "Buffer: " << buffer << endl;
            db<Persistent_Storage>(TRC) << "Data Size: " << data_control->_data_size << endl;
            db<Persistent_Storage>(TRC) << "Data Type: " << data_control->_data_type << endl;
            db<Persistent_Storage>(TRC) << "Data Version: " << data_control->_data_version << endl;
            db<Persistent_Storage>(TRC) << "Data ID: " << data_control->_data_id << endl;

            data_file = new RIFFS_DataFile;

            if(!this->write_data(data_control, buffer, data_file, base_address)) {
                delete(data_file);
                return 0;
            }

            _data_list->insert(new Simple_List<RIFFS_DataFile>::Element(data_file));

            db<Persistent_Storage>(TRC) << "Appended successfully!!!" << endl << endl;
        } while(size > 0);

        return 1;
    }

    void inspect_pieces(){
        db<Persistent_Storage>(TRC) << "Inspecting " << _data_list->size() << " pieces of " << _entry->_name << endl;
        RIFFS_DataFile *d;
        for(int i = 0; i < _data_list->size(); i++){
            d = _data_list->get(i)->object();
            db<Persistent_Storage>(TRC) << "Data File " << i+1 << endl;
            db<Persistent_Storage>(TRC) << "    * data_offset: " << d->_data_offset << endl;
            db<Persistent_Storage>(TRC) << "    * control_offset: " << d->_control_offset << endl;
            db<Persistent_Storage>(TRC) << "    * id: " << d->_id << endl;
            db<Persistent_Storage>(TRC) << "    * size: " << d->_size << endl;
            db<Persistent_Storage>(TRC) << "    * version: " << d->_version << endl;
            db<Persistent_Storage>(TRC) << "-------------------" << endl;

            char aux[d->_size];
            _device->read(d->_data_offset, aux, d->_size);

            db<Persistent_Storage>(TRC) << "    * Data: " << aux << endl << endl;
        }
    }

    uint32_t size(){
        uint32_t total_size = 0;

        for(int i = 0; i < _data_list->size(); i++)
            total_size += _data_list->get(i)->object()->_size;

        return total_size;
    }

    int remove() {
        db<Persistent_Storage>(TRC) << endl << "file::remove" << endl;

        uint32_t total_size = this->size();
        db<Persistent_Storage>(TRC) << "Size: " << total_size << ", pointer: " << _pointer << endl;
        db<Persistent_Storage>(TRC) << "Data has " << _data_list->size() << " pieces" << endl;

        db<Persistent_Storage>(TRC) << "File id is " << this->_id << endl;

        RIFFS_DataFile *data_file;
        RIFFS_FlashDataControl *data_control = new RIFFS_FlashDataControl;

        for(int i = 0; i < _data_list->size(); i++) {

            data_file = _data_list->get(i)->object();
            
            _device->read(data_file->_control_offset, data_control, sizeof(RIFFS_FlashDataControl));
            
            data_control->_data_type = RIFFS_FlashDataControl::DELETED;
            memset(data_control, 0, sizeof(data_control));

            uint32_t control_offset = data_file->_control_offset;
            
            _device->write(control_offset, data_control, sizeof(RIFFS_FlashDataControl));
        }

        delete data_control;
    }

    void read(char *buffer, uint32_t size){
        RIFFS_DataFile *data_file;

        db<Persistent_Storage>(TRC) << endl << "file::read" << endl;

        uint32_t total_size = this->size();
        db<Persistent_Storage>(TRC) << "Size: " << total_size << ", pointer: " << _pointer << endl;
        db<Persistent_Storage>(TRC) << "Data has " << _data_list->size() << " pieces" << endl;

        if(size > total_size)
            size = total_size;

        if(_pointer > total_size)
            return;

        int temp_pointer = _pointer;

        db<Persistent_Storage>(TRC) << endl << endl;

        uint32_t buffer_position = 0;

        for(int i = 0; i < _data_list->size(); i++) {

            data_file = _data_list->get(i)->object();

            //if _pointer is inside this first part of the file
            if( temp_pointer <= data_file->_size ) {
                
                db<Persistent_Storage>(TRC) << "  * Temp pointer: " << temp_pointer << endl;

                //check if all the read can be done within this part of the file
                if( temp_pointer + size <= data_file->_size ) {

                    db<Persistent_Storage>(TRC) << "All data can be read just once" << endl;
                    db<Persistent_Storage>(TRC) << "  * Offset + pointer: " << data_file->_data_offset + temp_pointer << endl;
                    db<Persistent_Storage>(TRC) << "  * Size to read: " << size << endl;
                    db<Persistent_Storage>(TRC) << "  * Data Size: " << data_file->_size << endl;

                    _device->read(data_file->_data_offset + temp_pointer, &(buffer[buffer_position]), size);

                    break;

                } else { //needs to move to the next part

                    db<Persistent_Storage>(TRC) << "NOT All data can be read just once" << endl;
                    db<Persistent_Storage>(TRC) << "  * Offset + pointer: " << data_file->_data_offset + temp_pointer << endl;
                    db<Persistent_Storage>(TRC) << "  * Size to read: " << size << endl;
                    db<Persistent_Storage>(TRC) << "  * Data Size: " << data_file->_size << endl;

                    _device->read(data_file->_data_offset + temp_pointer, buffer, (temp_pointer + size) - data_file->_size);

                    size = (size - data_file->_size) + temp_pointer;

                    buffer_position += data_file->_size - temp_pointer;

                    temp_pointer = 0;

                }

            } else {
                temp_pointer -= data_file->_size;
            }
        }
    }

    void seek(uint32_t position){
        if(position > this->size())
            return;

        _pointer = position;
    }
};
RIFFS_Allocator *RIFFS_File::_allocator;
Persistent_Storage *RIFFS_File::_device;

class RIFFS_FileManager{
protected:
    bool status_ok(RIFFS_FlashDataControl::Data_Status status) {
        if(status == RIFFS_FlashDataControl::TEMPORARY_WRITING
            || status == RIFFS_FlashDataControl::TEMPORARY_DELETE
            || status == RIFFS_FlashDataControl::DELETED) {
            return false;
        }

        return true;
    }

    bool type_ok(RIFFS_FlashDataControl::Type type) {

        if(type == RIFFS_FlashDataControl::SECTOR)
            return false;

        return true;
    }

    bool validate_files() {
        RIFFS_File *file;

        Simple_List<RIFFS_File>::Element *aux = _file_list.head();

        for(int i = 0; i < _file_list.size(); i++){
            file = aux->object();
            aux = aux->next();

            if(file->_entry) {
                if(file->_id >= file_counter_id)
                    file_counter_id = file->_id + 1;

                continue;
            }

            _file_list.remove(file);
            delete(file);
        }
    }
public:
    Simple_List<RIFFS_File> _file_list;
    Persistent_Storage *_device;
    RIFFS_Allocator *_allocator;
    RIFFS_Scanner *_scanner;
public:
    RIFFS_FileManager(Persistent_Storage *device, RIFFS_Allocator *allocator, RIFFS_Scanner *scanner) {
        _device = device;
        _allocator = allocator;
        _scanner = scanner;
    }

    int init(){
        db<Persistent_Storage>(TRC) <<endl << endl << endl<< endl << "***************************" << endl << "RIFFS_FileManager::init()" << endl;

        uint32_t base_address;
        RIFFS_MemoryDataControl *data_control;
        RIFFS_DataFile *data_file;
        RIFFS_File *file;
        Simple_List<RIFFS_DataFile> log_list;

        db<Persistent_Storage>(TRC) << "Checking [" << _scanner->data_list_count() << "] data" << endl;
        for(int i = 0; i < _scanner->data_list_count(); i++){
            data_control = _scanner->_data_list.get(i)->object();

            if(!status_ok((RIFFS_FlashDataControl::Data_Status) data_control->_data_version )) {
                db<Persistent_Storage>(TRC) << "This data does not have a valid status:" << endl;
                db<Persistent_Storage>(TRC) << "    * Current Version: " << data_control->_data_version;
                continue;
            }

            if(!type_ok((RIFFS_FlashDataControl::Type) data_control->_data_type )) {
                db<Persistent_Storage>(TRC) << "This data does not have a valid type:" << endl;
                db<Persistent_Storage>(TRC) << "    * Type: " << data_control->_data_type;
                continue;
            }

            db<Persistent_Storage>(TRC) << "File is valid:" << endl;
            db<Persistent_Storage>(TRC) << "    * Sector ID: " << data_control->_sector_id << endl;
            db<Persistent_Storage>(TRC) << "    * Data ID: " << data_control->_data_id << endl;
            db<Persistent_Storage>(TRC) << "    * Control Offset: " << data_control->_control_offset << endl;
            db<Persistent_Storage>(TRC) << "    * Data offset: " << data_control->_data_offset + data_control->_sector_id*_device->page_size()<< endl;
            db<Persistent_Storage>(TRC) << "    * Data Size: " << data_control->_data_size << endl;
            db<Persistent_Storage>(TRC) << "    * Data version: " << data_control->_data_version << endl;

            if(!(file = get_file_id(data_control->_data_id))) {
                file = new RIFFS_File(data_control->_data_id);
                _file_list.insert(new Simple_List<RIFFS_File>::Element(file));
            }

            data_file = new RIFFS_DataFile;
            data_file->init(data_control->_data_id,
                data_control->_data_offset + data_control->_sector_id*_device->page_size(),
                data_control->_control_offset,
                data_control->_data_size,
                data_control->_data_version);

            switch(data_control->_data_type) {
                case RIFFS_FlashDataControl::DATA:
                    db<Persistent_Storage>(TRC) << "Data type: data" << endl;
                    file->_data_list->insert(new Simple_List<RIFFS_DataFile>::Element(data_file));
                    break;
                case RIFFS_FlashDataControl::LOG_CONTEXT:
                    db<Persistent_Storage>(TRC) << "Data type: log context" << endl;
                    log_list.insert(new Simple_List<RIFFS_DataFile>::Element(data_file));
                    break;
                case RIFFS_FlashDataControl::ENTRY_CONTEXT:
                    db<Persistent_Storage>(TRC) << "Data type: entry context" << endl;
                    if((!file->_entry || file->_entry->_version <= data_file->_version))
                        file->entry(data_file);
                    break;
            }
            db<Persistent_Storage>(TRC) << endl;
        }

        return 1;
    }

    uint32_t get_file_count(){ return _file_list.size(); }

    int validate_log(RIFFS_File *file, RIFFS_DataFile *log_data);
    int delete_file(RIFFS_File *file);
    int close_file(RIFFS_File *file);

    RIFFS_File *get_file_id(uint32_t file_id) {
        RIFFS_File *file;
        for(int i = 0; i < _file_list.size(); i++){
            file = _file_list.get(i)->object();

            if(file->_id == file_id)
                return file;
        }

        return 0;
    }

    void remove_file(RIFFS_File *file) {
        db<Persistent_Storage>(TRC) << endl << "RIFFS_FileManager::remove_file" << endl;

        RIFFS_MemoryDataControl *data_control;

        for(int i = 0; i < _scanner->data_list_count(); i++){
            data_control = _scanner->_data_list.get(i)->object();

            if( file->_id == data_control->_data_id && data_control->_data_type == RIFFS_FlashDataControl::ENTRY_CONTEXT ) {
                
                
                uint32_t doffset = data_control->_control_offset;
                memset(data_control, 0, sizeof(data_control));

                _device->write(doffset, data_control, sizeof(RIFFS_FlashDataControl));

                file->remove();

                return;
            }
        }
    }

    RIFFS_File *create_file(const char *name, uint32_t father, uint32_t name_size) {
        RIFFS_Entry entry;

        strncpy(entry._name, name, name_size);

        entry._father_id = father;
        entry._type = RIFFS_Entry::FILE;

        db<Persistent_Storage>(TRC) << endl << "FileManager::create_file()" << endl;
        db<Persistent_Storage>(TRC) << "    * Father id is: " << entry._father_id << endl;
        db<Persistent_Storage>(TRC) << "    * Type is: 0x" << hex << entry._type << endl;
        db<Persistent_Storage>(TRC) << "    * Name is: " << entry._name << endl;

        RIFFS_File *file = RIFFS_File::create_entry(&entry, _device, _allocator, name_size);

        return file;
    }

    RIFFS_File *open_file(const char *name, uint32_t father_id) {

        for(int i = 0; i < _file_list.size(); i++){
            RIFFS_File *file = _file_list.get(i)->object();

            if(file->_entry->_father_id == father_id && strcmp(name, file->_entry->_name) == 0) {
                db<Persistent_Storage>(TRC) << endl << "File found: " << endl;
                db<Persistent_Storage>(TRC) << "    * Father ID: " << file->_entry->_father_id << endl;
                db<Persistent_Storage>(TRC) << "    * Type: 0x" << hex << file->_entry->_type << endl;
                db<Persistent_Storage>(TRC) << "    * Version: " << dec << file->_entry->_version << endl;
                db<Persistent_Storage>(TRC) << "    * Name: " << file->_entry->_name << endl;

                db<Persistent_Storage>(TRC) << "-------------------" << endl;
                db<Persistent_Storage>(TRC) << "This file has " << file->_data_list->size() << " pieces." << endl;

                return file;
            }
        }

        db<Persistent_Storage>(TRC) << "Requested file was not found" << endl;
        return 0;
    }

    RIFFS_File *open_file(uint32_t id, uint32_t father_id) {

    }
};
/************************************************************/

/*************************DIR BEGINS*************************/
class RIFFS_Node : public RIFFS_EntryFile {
public:
    Simple_List<RIFFS_Node> *_directory_list;
    Simple_List<RIFFS_File> *_file_list;

public:
    RIFFS_Node(){
        _directory_list = 0;
        _file_list = 0;
    }

    int link(RIFFS_EntryFile *entry_file);
};

class RIFFS_DirectoryManager{
private:
    void recursive(RIFFS_Node *node) {
        db<Persistent_Storage>(TRC) << "Directory Manager recursive init" << endl;

        if(node->_directory_list == 0)
            node->_directory_list = new Simple_List<RIFFS_Node>;

        if(node->_file_list == 0)
            node->_file_list = new Simple_List<RIFFS_File>;

        RIFFS_File *this_node;

        for(int i = 0; i < _file_manager->_file_list.size(); i++) {

            this_node = _file_manager->_file_list.get(i)->object();

            if(this_node->_entry->_father_id != node->_id)
                continue;


            if(this_node->_entry->_type == RIFFS_Entry::FILE) {

                db<Persistent_Storage>(TRC) << "File..." << endl;
                node->_file_list->insert(new Simple_List<RIFFS_File>::Element(this_node));

            } else {

                db<Persistent_Storage>(TRC) << "Directory..." << endl;
                node->_directory_list->insert(new Simple_List<RIFFS_Node>::Element((RIFFS_Node *) this_node));

            }

            recursive((RIFFS_Node *) this_node);
        }
    }

public:
    RIFFS_FileManager *_file_manager;
    RIFFS_Node *_root;

public:
    RIFFS_DirectoryManager(RIFFS_FileManager *file_manager) {
        _file_manager = file_manager;

        _root = new RIFFS_Node;
        _root->_entry = new RIFFS_ClassifiedEntry;
        _root->_entry->_father_id = 0;
        _root->_entry->_type = RIFFS_Entry::DIRECTORY;
        _root->_entry->_version = 0;
        _root->_id = 1;
        _root->_entry->_name[0] = '/';
        _root->_entry->_name[1] = '\0';
        _root->_directory_list = new Simple_List<RIFFS_Node>;
        _root->_file_list = new Simple_List<RIFFS_File>;
    }

    int init(Persistent_Storage *device, RIFFS_Scanner *scanner) {
        db<Persistent_Storage>(TRC) << endl << "----------------------------------------------" << endl;
        db<Persistent_Storage>(TRC) << "RIFFS_DirectoryManager::init()" << endl;

        RIFFS_File *this_node;

        if(_file_manager->_file_list.size() > 0)
            this_node = _file_manager->_file_list.get(0)->object();
        else
            return 0;

        RIFFS_Node *last_node = _root;

        recursive(last_node);


        //file_list_size--;

        while(_file_manager->_file_list.size() > 0)
            _file_manager->_file_list.remove_head();

        db<Persistent_Storage>(TRC) << "RIFFS_DirectoryManager::init(DONE)" << endl << endl << endl;
        return 1;
    }

    void validate_files() {
        RIFFS_File *file;

        for(int i = 0; i < _file_manager->_file_list.size(); i++){
            file = _file_manager->_file_list.get(i)->object();

            _file_manager->_file_list.remove(file);
            delete(file);
        }
    }

    RIFFS_Node *create_directory(char *name, RIFFS_Node *father) {
        RIFFS_Entry entry;

        int name_size = 0;

        RIFFS_Common::linearize_char(name, &name_size, 0);

        strcpy(entry._name, name);

        entry._father_id = father->_id;
        entry._type = RIFFS_Entry::DIRECTORY;

        RIFFS_File *file = RIFFS_File::create_entry(&entry, RIFFS_File::_device, RIFFS_File::_allocator, name_size);

        if(!file)
            return 0;

        father->_file_list->insert(new Simple_List<RIFFS_File>::Element(file));

        return ((RIFFS_Node*) file);
    }
};
/************************************************************/

typedef RIFFS_File File;

class RIFFS {
private:

    int get_end_of_name(const char *name, int begin){
        while(name[begin] != '/' && name[begin] != '\0' )
            begin++;

        return begin;
    }

public:
    RIFFS(){
    }

    void mount(){
        _device = new Persistent_Storage;
        _scanner = new RIFFS_Scanner(_device);

        db<Persistent_Storage>(TRC) << "RIFFS::mount(device_address=" << _device << ")" << endl;

        _scanner->scan();

        _allocator = new RIFFS_Allocator(_scanner);

        _file_manager = new RIFFS_FileManager(_device, _allocator, _scanner);
        _file_manager->init();

        file_counter_id = file_counter_id + _file_manager->_file_list.size() + 1;

        _dir_manager = new RIFFS_DirectoryManager(_file_manager);
        _dir_manager->init(_device, _scanner);
    }

    void format() {
        db<Persistent_Storage>(TRC) << "RIFFS:format()" << endl;

        uint32_t sector_address;
        uint32_t sector_size;
        RIFFS_SectorHeader header;
        RIFFS_FlashDataControl data_control;

        int num_of_pages = _device->pages();

        for(int i = 0; i < num_of_pages; i++){
            _device->get_sector_address(i, &sector_address, &sector_size);

            db<Persistent_Storage>(TRC) << "--------------------------------------" << endl;

            bool isHeaderValid = RIFFS_Common::get_header(_device, &header, sector_address);

            if(isHeaderValid) {
                db<Persistent_Storage>(TRC) << "Sector [" << (i + 1) << "] is valid!" << endl;
            } else {
                db<Persistent_Storage>(TRC) << "Sector [" << (i + 1) << "] is not valid!" << endl;
                header._magic_number = MAGIC_NUMBER;
                header._erased_times = 0;
                header._sector_size = _device->page_size();

                _device->write(sector_address, &header, sizeof(RIFFS_SectorHeader));
            }

            header._erased_times += 1;

            char clean[_device->page_size() - sizeof(RIFFS_SectorHeader) - sizeof(RIFFS_FlashDataControl)];
            memset(clean, 1, sizeof(clean));

            data_control._data_offset = sector_address;
            data_control._data_size = sizeof(RIFFS_SectorHeader);
            data_control._data_id = i;
            data_control._data_type = RIFFS_FlashDataControl::SECTOR;
            data_control._data_version = CURRENT_VERSION;

            uint32_t writing_address = sector_address + sector_size - sizeof(RIFFS_FlashDataControl);

            _device->write(sector_address + sizeof(RIFFS_SectorHeader), clean, sizeof(clean));

            _device->write(writing_address, &data_control, sizeof(RIFFS_FlashDataControl));

            _device->write(sector_address, &header, sizeof(RIFFS_SectorHeader));

            db<Persistent_Storage>(TRC) << "Done formatting for sector [" << (i + 1) << "]" << endl;
            db<Persistent_Storage>(TRC) << "Sector was erased [" << header._erased_times << "] times" << endl;

            db<Persistent_Storage>(TRC) << "--------------------------------------" << endl << endl;;
        }
    }

    File * create(char *name, uint32_t father_id){
        db<Persistent_Storage>(TRC) << endl << endl << "****************************************************" << endl;
        db<Persistent_Storage>(TRC) << "RIFFS::create_file()" << endl;

        RIFFS_File *file;

        int name_size = 0;

        RIFFS_Common::linearize_char(name, &name_size, 0);

        file = _file_manager->create_file(name, father_id, name_size);

        return file;
    }

    File *open(const char *name){
        db<Persistent_Storage>(TRC) << "Opening File: " << name << endl;

        RIFFS_Node *temp;

        RIFFS_Node *last_node = _dir_manager->_root;

        int pos = 0;
        if(name[pos] != '/')
            return 0;

        bool is_file = false;

        pos++;

        while(!is_file) {

            int size_to_compare = get_end_of_name(name, pos);

            is_file = (name[size_to_compare] == '/')?false:true;

            db<Persistent_Storage>(TRC) << "It is a file? " << is_file << endl;

            int iteration_size = (is_file)?last_node->_file_list->size():last_node->_directory_list->size();

            db<Persistent_Storage>(TRC) << "This item has " << iteration_size << " elements." << endl;

            if(iteration_size == 0) {
                return 0; //wrong path or file not found
            }

            for(int i = 0; i < iteration_size; i++) {
                if(is_file) {
                    temp = (RIFFS_Node *) last_node->_file_list->get(i)->object();

                    int match = strncmp(&(name[pos]), temp->_entry->_name, size_to_compare - pos);
                    if( match == 0 ) {
                        return last_node->_file_list->get(i)->object();
                    }
                } else {
                    temp = last_node->_directory_list->get(i)->object();
                    int match = strncmp(&(name[pos]), temp->_entry->_name, size_to_compare - pos);
                    if( match == 0 ) {
                        pos = size_to_compare + 1;

                        last_node = temp;
                        break;
                    }
                }

                if(i == iteration_size - 1)
                    return 0;
            }
        }
    }

    File * create_dir(const char *name){
        return (RIFFS_File *) _dir_manager->create_directory((char *) name, _dir_manager->_root);
    }

    uint32_t get_available_space(){
        uint32_t size = 0;
        uint32_t count = 0;
        for(int i = 0; i < _scanner->_sector_list.size(); i++){
            size += _scanner->available_space(i, &count);
        }

        return size;
    }

    void remove(File *file) {
        db<Persistent_Storage>(TRC) << "Removing file " << file->_entry->_name << endl;

        _file_manager->remove_file(file);
    }

private:
    Persistent_Storage *_device;

    RIFFS_Scanner *_scanner;
    RIFFS_Allocator *_allocator;
    RIFFS_FileManager *_file_manager;
    RIFFS_DirectoryManager *_dir_manager;
};

typedef RIFFS FileSystem;

__END_SYS

#endif
