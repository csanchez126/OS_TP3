#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "conf.h"
#include "common.h"
#include "vmm.h"
#include "tlb.h"
#include "pt.h"
#include "pm.h"

static unsigned int read_count = 0;
static unsigned int write_count = 0;
static FILE* vmm_log;

void vmm_init (FILE *log)
{
  // Initialise le fichier de journal.
  vmm_log = log;
}

// Un test
// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command (FILE *out, const char *command,
                             unsigned int laddress, /* Logical address. */
		             unsigned int page,
                             unsigned int frame,
                             unsigned int offset,
                             unsigned int paddress, /* Physical address.  */
		             char c) /* Caractère lu ou écrit.  */
{
  if (out)
    fprintf (out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c, laddress,
	     page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read (unsigned int laddress)
{
  char c = '!';
  read_count++;
  /* ¡ TODO: COMPLÉTER ! */

  // Traduction de l'addresse en format page et offset
  int page = laddress >> 8;
  int offset = laddress & 0xff;

  int frame = tlb_lookup(page, false);

  if(frame>-1){ //le frame est dans le TLB
	  int pm_add = (frame << 8)+offset;
	  c = pm_read(pm_add);
  }else if(pt_lookup(page) > 1){
	  /* TODO Check page table*/
	  int pm_add = (pt_lookup(page) << 8)+offset;
	  c = pm_read(pm_add);
  }else{

	  /* TODO Not loaded in pt, get from backing store
	   * and update PT, TLB
	   * */
  }



  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "READING", laddress, 0, 0, 0, 0, c);
  return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write (unsigned int laddress, char c)
{
  write_count++;
  /* ¡ TODO: COMPLÉTER ! */

  pm_download_page(0,0);
  pm_write(0, 'T');
  pm_write(1,'e');
  pm_write(2, 's');
  pm_write(3, 't');
  pm_backup_frame(0,0);
  // TODO: Fournir les arguments manquants.
  vmm_log_command (stdout, "WRITING", laddress, 0, 0, 0, 0, c);
}


// NE PAS MODIFIER CETTE FONCTION
void vmm_clean (void)
{
  fprintf (stdout, "VM reads : %4u\n", read_count);
  fprintf (stdout, "VM writes: %4u\n", write_count);
}
