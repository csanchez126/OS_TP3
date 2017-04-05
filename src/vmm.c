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

void vmm_init(FILE *log) {
	// Initialise le fichier de journal.
	vmm_log = log;

}

// Un test
// NE PAS MODIFIER CETTE FONCTION
static void vmm_log_command(FILE *out, const char *command,
		unsigned int laddress, /* Logical address. */
		unsigned int page, unsigned int frame, unsigned int offset,
		unsigned int paddress, /* Physical address.  */
		char c) /* Caractère lu ou écrit.  */
{
	if (out)
		fprintf(out, "%s[%c]@%05d: p=%d, o=%d, f=%d pa=%d\n", command, c,
				laddress, page, offset, frame, paddress);
}

/* Effectue une lecture à l'adresse logique `laddress`.  */
char vmm_read(unsigned int laddress) {
	char c = '!';
	read_count++;
	/* ¡ TODO: COMPLÉTER ! */

	// Traduction de l'addresse en format page et offset
	int page = laddress >> 8;
	int offset = laddress & 0xff;

	int frame = tlb_lookup(page, false);
	int pm_add;
	if (frame > -1) { //le frame est dans le TLB
		printf("TLB HIT\n");

	} else if (pt_lookup(page) > -1) { //Pas dans le TLB, on regarde dans le PT
		printf("TLB MISS, checking PT\n");
		frame = pt_lookup(page);
		tlb_add_entry(page, frame, true);

		//TODO: UPDATE TLB
	} else {
		/* Frame not loaded in PT either, get from backing
		 * store and update PT, TLB */
		//1. Find a frame in PM
		//2. Is the frame free? Read into it from backing store
		//3. Else frame not free
		//	3.1 Read only? Read from backing store into frame
		//	3.2 Page has been modified, upload to disk before reading
		frame = pm_find_victim_pm_frame();
		printf("TLB and PT MISS, GOT FRAME: %d\n", frame);
		//Check if the frame is dirty so we can backup
		//the associated frame to BACKING_STORE
		if (pm_getDirtyBit(frame) == true) {
			int pageBackup = pm_getLoadedPage(frame); //Get page to swap out
			printf("backing up frame %d to page %d\n", frame, pageBackup);
			pm_backup_frame(frame, pageBackup);
			pt_unset_entry(pageBackup); //Tell PT the page is not in PM anymore
		}
		pm_download_page(page, frame); //Get page from BACKING_STORE
		pt_set_entry(page, frame); //Update PT
		tlb_add_entry(page, frame, true);

	}

	pm_update_lru(frame);
	pm_setDirtyBit(frame, false);

	pm_add = (frame << 8) + offset;
	c = pm_read(pm_add);

	vmm_log_command(stdout, "READING", laddress, page, frame, offset, pm_add,
			c);
	return c;
}

/* Effectue une écriture à l'adresse logique `laddress`.  */
void vmm_write(unsigned int laddress, char c) {
	write_count++;
	/* ¡ TODO: COMPLÉTER ! */
	// Traduction de l'addresse en format page et offset
	int page = laddress >> 8;
	int offset = laddress & 0xff;

	int frame = tlb_lookup(page, true);
	int pm_add;



	if (frame > -1) { //le frame est dans le TLB

		printf("TLB HIT\n");
	} else if (pt_lookup(page) > -1) { //Pas dans le TLB, on regarde dans le PT
		printf("TLB MISS, checking PT\n");
		frame = pt_lookup(page);
		tlb_add_entry(page, frame, false);
	} else {
		/* Frame not loaded in PT either, get from backing
		 * store and update PT, TLB */
		//1. Find a frame in PM
		//2. Is the frame free? Read into it from backing store
		//3. Else frame not free
		//	3.1 Read only? Read from backing store into frame
		//	3.2 Page has been modified, upload to disk before reading
		frame = pm_find_victim_pm_frame();
		printf("TLB and PT MISS, GOT FRAME: %d\n", frame);
		pm_add = (frame << 8) + offset;
		//Check if the frame is dirty so we can backup
		//the associated frame to BACKING_STORE
		if (pm_getDirtyBit(frame) == true) {
			int pageBackup = pm_getLoadedPage(frame); //Get page to swap out
			printf("backing up frame %d to page %d\n", frame, pageBackup);
			pm_backup_frame(frame, pageBackup);
			pt_unset_entry(pageBackup); //Tell PT the page is not in PM anymore
		}
		pm_download_page(page, frame); //GET PAGE FROM BACKING STORE
		pt_set_entry(page, frame); //Update PT
		tlb_add_entry(page, frame, false);

	}

	pm_update_lru(frame);
	pm_setDirtyBit(frame, true);

	pm_add = (frame << 8) + offset;
	pm_write(pm_add, c);


	vmm_log_command(stdout, "WRITING", laddress, page, frame, offset, pm_add,
			c);
}

// NE PAS MODIFIER CETTE FONCTION
void vmm_clean(void) {
	fprintf(stdout, "VM reads : %4u\n", read_count);
	fprintf(stdout, "VM writes: %4u\n", write_count);
}
