/*****************************************************************
 * Source file : filesystem.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.0.2 (roadrunner) on 22/10/2008 15:49
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#include <l4io.h>
#include <idl4glue.h>
#include <string.h>
#include "filesystem.h"
#include "filesystem-server.h"

typedef struct {
    L4_Word_t * position; // Position und Größe sollte immer so gewählt werden,
    L4_Word_t   size;     // dass man das ganze in eine Fpage packen kann.
    bool        valid;
    char        name[16];
} filehandle_t;

// Es gibt bis zu 256 Dateien im System
filehandle_t filehandles[MAXFILE + 1];

// Unser Heap geht bis 1 GB, d.h. das Dateisystem beginnt
// bei 1 GB
L4_Word_t * beginOfFilesystem = (L4_Word_t *)0x40000000;       // 1 GB
L4_Word_t * filesystemPointer = beginOfFilesystem;  // Hier kann die nächste Datei hingelegt werden

// **** HILFSFUNKTIONEN *****
// Alle Filehandles initialisieren
void initFilehandles(){
    for(int i = 0; i < MAXFILE; i++){
        filehandles[i].position = 0;
        filehandles[i].size     = 0;
        filehandles[i].valid    = 0;
        filehandles[i].name[0]  = (char)0; 
    }
}

// Alle Filehandles anzeigen, fuer Debugging
void dumpFilehandles(){
    for(int i = 0; i < MAXFILE; i++)
        if(filehandles[i].valid)
            printf("P: %i S: %i V: %i N: %s\n", filehandles[i].position, filehandles[i].size, \
                                                filehandles[i].valid, filehandles[i].name);
}

// Liefert den Filehandle zu einem eingegebenen Namen
int locateFilehandle(char * filename){
    printf("[SIMPLE-FS] Entering locate Filehandle...\n");
    for(int i = 0; i < MAXFILE; i++){
        printf("[SIMPLE-FS] strcmp: %i valid: %i\n", strcmp(filehandles[i].name, filename), filehandles[i].valid); 
        if(filehandles[i].valid && (strcmp(filehandles[i].name, filename) == 0)){
            return i;
        }
    }

    return -1; // Nichts passendes gefunden
}

// Freies Filehandle suchen
int locateFreeFilehandle(){
    for(int i = 0; i < MAXFILE; i++)
        if(filehandles[i].valid == 0)
            return i; // Freies Filehandle gefunden

    // Konnte kein freies Filehandle finden
    return -1;
}
// **** ENDE DER HILFSFUNKTIONEN ****


/* Interface filesystem */

IDL4_INLINE void filesystem_listFiles_implementation(CORBA_Object _caller, buffer_t *filenames, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::listFiles */

    // 1: Wir müssen uns erstmal Filenames in irgend eine Form bringen, mit
    // der wir arbeiten können.
    filenames->_maximum = (MAXFILE + 1) * 16;
    filenames->_length  = (MAXFILE + 1) * 16;
    char * retPos       = (char *)filenames->_buffer; 

    // String komplett säubern:
    memset(retPos, 0, (MAXFILE + 1) * 16);
 
    // 2: Kopieren aller Dateinamen von Filehandles, die Valid sind
    for(int i = 0; i < MAXFILE; i++)
        if(filehandles[i].valid == 1){
            strncpy(retPos, filehandles[i].name, 15);
            memset(retPos + 15, 0, 1);
            retPos += 16;
        }

    return;
}

IDL4_PUBLISH_FILESYSTEM_LISTFILES(filesystem_listFiles_implementation);


IDL4_INLINE void filesystem_mapFile_implementation(CORBA_Object _caller, const CORBA_char *filename, idl4_fpage_t *page, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::mapFile */

    // 1: Filehandle suchen
    int i = locateFilehandle((char *)filename);

    if( (i < 0) ){
         CORBA_exception_set(_env, ex_IF_FILESYSTEM_fileNotFound, 0);
         return;
    }

    // 2: Passende Fpage erzeugen
    L4_Fpage_t fpage = L4_Fpage((L4_Word_t)filehandles[i].position, filehandles[i].size);  

    // 3: Fpage zurückliefern

    idl4_fpage_set_mode(page, IDL4_MODE_MAP);
    idl4_fpage_set_page(page, fpage);
    //idl4_set_base(page, page_end);
    idl4_fpage_set_permissions(page, IDL4_PERM_FULL);
  
    return;
}

IDL4_PUBLISH_FILESYSTEM_MAPFILE(filesystem_mapFile_implementation);

IDL4_INLINE void filesystem_createFile_implementation(CORBA_Object _caller, const CORBA_char *filename, const L4_Word_t size, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::createFile */

    printf("[SIMPLE-FS] Trying to create file '%s' of size %i... ", filename, size);
    
    // 1: Größe so zurrechtbiegen, dass die Datei später in eine Fpage passt
    L4_Word_t mysize = size & (0xFFFFF000);
    
    // Wir wollen ja nicht, dass die Dateien zu klein werden...
    while(mysize < size)
        mysize *= 2;
   
    // 2. filesystemPointer alignen -- Überflüssig, da mysize schon aligned
//    filesystemPointer =  (L4_Word_t *)((L4_Word_t)filesystemPointer & ((0xFFFF0000) + (0x1000)));

    // überprüfen, ob die Datei bereits existiert
    if(locateFilehandle((char *)filename) > -1){
        printf("[SIMPE-FS] ... error: File already exists!\n");
        CORBA_exception_set(_env, ex_IF_FILESYSTEM_fileExists, 0);
        return;
    }

    // Überprüfen, ob noch genügend Speicher verfügbar ist
    if( ((L4_Word_t)filesystemPointer + mysize) >= (0xc0000000) ){ 
        CORBA_exception_set(_env, ex_IF_FILESYSTEM_outOfMemory, 0);
        return;
    }

    // 3. Freies Filehandle finden
    int i = locateFreeFilehandle();

    if(i < 0){
        CORBA_exception_set(_env, ex_IF_FILESYSTEM_outOfFilehandles, 0); // Es wurden keine freien Filehandles gefunden
        return;
    }

    // 4. Filehandle schreiben
    filehandles[i].position = filesystemPointer;
    filehandles[i].size     = mysize;
    filehandles[i].valid    = 1;
    strncpy(filehandles[i].name, filename, 16);

    // 5. filesystemPointer erhöhen
    filesystemPointer += mysize / 4; // Durch 4, da Seiten.

    printf("... created at position %i!\n", i);
    printf("Filesystem pointer incremented by %lx\n", mysize);

    dumpFilehandles();

    return;
}

IDL4_PUBLISH_FILESYSTEM_CREATEFILE(filesystem_createFile_implementation);

IDL4_INLINE void filesystem_deleteFile_implementation(CORBA_Object _caller, const CORBA_char *filename, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::deleteFile */

    // 1: Filehandle dieser Datei suchen 
    int i = locateFilehandle((char *)filename);

    if(i < 0){
        printf("[SIMPE-FS] Error: fileNotFound!\n");
        CORBA_exception_set(_env, ex_IF_FILESYSTEM_fileNotFound, 0); // Datei wurde nicht gefunden
        return;
    }

    // 2: Datei als gelöscht markieren
    filehandles[i].valid = 0;
  
  return;
}

IDL4_PUBLISH_FILESYSTEM_DELETEFILE(filesystem_deleteFile_implementation);

void *filesystem_vtable_10[FILESYSTEM_DEFAULT_VTABLE_SIZE] = FILESYSTEM_DEFAULT_VTABLE_10;
void *filesystem_vtable_discard[FILESYSTEM_DEFAULT_VTABLE_SIZE] = FILESYSTEM_DEFAULT_VTABLE_DISCARD;
void **filesystem_itable[16] = { filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_10, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard, filesystem_vtable_discard };

void filesystem_server(void)

{
  L4_ThreadId_t partner;
  L4_MsgTag_t msgtag;
  idl4_msgbuf_t msgbuf;
  long cnt;

  // initialize Filehandles
  initFilehandles();

  idl4_msgbuf_init(&msgbuf);
  for (cnt = 0;cnt < FILESYSTEM_STRBUF_SIZE;cnt++)
    idl4_msgbuf_add_buffer(&msgbuf, malloc(8000), 8000);

  while (1)
    {
      partner = L4_nilthread;
      msgtag.raw = 0;
      cnt = 0;

      while (1)
        {
          idl4_msgbuf_sync(&msgbuf);

          idl4_reply_and_wait(&partner, &msgtag, &msgbuf, &cnt);

          if (idl4_is_error(&msgtag))
            break;

          idl4_process_request(&partner, &msgtag, &msgbuf, &cnt, filesystem_itable[idl4_get_interface_id(&msgtag) & FILESYSTEM_IID_MASK][idl4_get_function_id(&msgtag) & FILESYSTEM_FID_MASK]);
        }
    }
}

void filesystem_discard(void)

{
}

