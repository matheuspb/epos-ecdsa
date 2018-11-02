#include <utility/ostream.h>
#include <alarm.h>
#include <gpio.h>
#include <riffs.h>

using namespace EPOS;

FileSystem fs;
OStream cout;

class FileRecord {
public:
    FileRecord(){
        for(int i = 0; i < 64; i++)
            name[i] = 'A';
    }
    unsigned int a;
    unsigned int b;
    unsigned int c;
    unsigned int d;
    unsigned short e;
    char name[64];
};

void read(char *n) {
    FileRecord test;
    File *file = fs.open((char *) n);

    cout << "Reading file: " << n << endl;
    cout << "File size is: " << file->size() << endl;

    file->seek(0);

    file->read((char *) &test, sizeof(FileRecord));

    cout << "test.a = " << test.a << endl;
    cout << "test.b = " << test.b << endl;
    cout << "test.c = " << test.c << endl;
    cout << "test.d = " << test.d << endl;
    cout << "test.e = " << test.e << endl;
    cout << "test.name = " << test.name << endl;
}

void append(char *n) {
    File *f = fs.open(n);

    FileRecord test;
    test.a = 50;
    test.b = 51;
    test.c = 52;
    test.d = 53;
    test.e = 54;

    char buffer[sizeof(FileRecord)];

    for(int i = 0; i < sizeof(FileRecord); i++) {
        buffer[i] = reinterpret_cast<volatile char *>(&test)[i];
    }

    int result = f->append(buffer, sizeof(FileRecord));

    cout << endl << "File appended" << endl;
}

int main(){

    Delay(1000000);
    cout << "RIFFS Example" << endl;

    /*
        The flash must be formatted according to the RIFFS file system standard

        fs.format();
    */

    /*
        In order to use the file system, the device must be mounted!
        If an external device is used for the flash it can be passed
        as parameter here as long as the persistent_storage
        class has the same interface as the one implemented.

        IF FILE SYSTEM IS NOT MOUNTED, IT WON'T WORK PROPERLY
    */
    fs.mount();

    cout << "Available Flash Space is " << fs.get_available_space() << " bytes." << endl;

    /*
        To create a file, its father id has to be passed as parameter
        The father id relates the file with its owner

        The root directory id is 1 as default
        So far all the directories are added to the root dir and any file
        can be added to root or sub directories

        File *dir = fs.create_dir("test_directory");
        File *file = fs.create("test_file.txt", dir->_id);
    */

    /*
        To open the file the correct path must be passed.
        It is important to note that the '/' on the beginning represents
        the root directory. So if it is not added to the path, the file won't be found
    */
    File *file = fs.open("/test_directory/test_file.txt");
    cout << "File was found? " << (file!=0) << endl; //if pointer == 0, the file was not found on the system
    cout << "Reading file: /test_directory/test_file.txt" << endl;
    if(file != 0)
        cout << "File size is: " << file->size() << endl;

    /*
        The following methods data to a file and reads data from a file as well

        append("/test_directory/test_file.txt");
        read("/test_directory/test_file.txt");
    */

    while(1){
        Delay(1000000);
    }

    return 0;
}
