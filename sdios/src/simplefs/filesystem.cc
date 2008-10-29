/*****************************************************************
 * Source file : filesystem.idl
 * Platform    : V4 IA32
 * Mapping     : CORBA C
 * 
 * Generated by IDL4 1.0.2 (roadrunner) on 22/10/2008 15:49
 * Report bugs to haeberlen@ira.uka.de
 *****************************************************************/

#include "filesystem.h"
#include "filesystem-server.h"

typedef struct {
    L4_Word_t   position; // Position und Größe sollte immer so gewählt werden,
    L4_Word_t   size;     // dass man das ganze in eine Fpage packen kann.
    bool        valid;
    char        name[16];
} filehandle_t;

// Es gibt bis zu 256 Dateien im System
filenhandle_t filehandles[MAXFILE + 1];

// Unser Heap geht bis 1 GB, d.h. das Dateisystem beginnt
// bei 1 GB
beginOfFilesystem = (0x40000000);       // 1 GB
filesystemPointer = beginOfFilesystem;  // Hier kann die nächste Datei hingelegt werden

// **** HILFSFUNKTIONEN *****
// Liefert den Filehandle zu einem eingegebenen Namen
int locateFilehandle(char filename[16]){
    for(int i = 0; i < MAXFILE; i++){
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
    retFilenames = (char [MAXFILE + 1][16])filenames;
    retPos       = 0;
 
    // 2: Kopieren aller Dateinamen von Filehandles, die Valid sind
    for(int i = 0; i < MAXFILE; i++)
        if(filehandles[i].valid)
            strcpy(retFilenames[retPos++], filehandles[i].name);

    return;
}

IDL4_PUBLISH_FILESYSTEM_LISTFILES(filesystem_listFiles_implementation);


IDL4_INLINE void filesystem_mapFile_implementation(CORBA_Object _caller, const CORBA_char *filename, idl4_fpage_t *page, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::mapFile */

    // 1: Filehandle suchen
    int i = locateFilehandle(filename);

    if( (i < 0) )
        raise fileNotFound;

    // 2: Passende Fpage erzeugen
    fpage = L4_Fpage(filehandles[i].position, filehandles[i].size);  

    // 3: Fpage zurückliefern
    page = fpage;
  
    return;
}

IDL4_INLINE void filesystem_createFile_implementation(CORBA_Object _caller, const CORBA_char *filename, const L4_Word_t size, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::createFile */
    
    // 1: Größe so zurrechtbiegen, dass die Datei später in eine Fpage passt
    size &= (0xFFFF0000) + (0x1000); // Datei ist immer mindestens 4 kB Groß
   
    // 2. filesystemPointer alignen
    filesystemPointer &= (0xFFFF0000) + (0x1000);

    // Überprüfen, ob noch genügend Speicher verfügbar ist
    if( (filesystemPointer + size) >= (0xc0000000) ) 
        raise outOfMemory;

    // 3. Freies Filehandle finden
    int i = locateFreeFilehandle();

    if(i < 0)
        raise outOfFilehandles; // Es wurden keine freien Filehandles gefunden

    // 4. Filehandle schreiben
    filehandles[i].position = filesystemPointer;
    filehandles[i].size     = size;
    filehandles[i].valid    = 1;
    strcpy(filehandles[i].name, filename);

    // 5. filesystemPointer erhöhen
    filesystemPointer += size;
  
    return;
}

IDL4_PUBLISH_FILESYSTEM_CREATEFILE(filesystem_createFile_implementation);

IDL4_INLINE void filesystem_deleteFile_implementation(CORBA_Object _caller, const CORBA_char *filename, idl4_server_environment *_env)

{
    /* implementation of IF_FILESYSTEM::deleteFile */

    // 1: Filehandle dieser Datei suchen 
    int i = locateFilehandle(filename);

    if(i < 0)
        raise fileNotFound; // Datei wurde nicht gefunden

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
