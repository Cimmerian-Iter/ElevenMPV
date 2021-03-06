#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "dirbrowse.h"
#include "fs.h"
#include "menu_audioplayer.h"
#include "textures.h"
#include "utils.h"

File *files = NULL;

static void Dirbrowse_RecursiveFree(File *node) {
	if (node == NULL) // End of list
		return;
	
	Dirbrowse_RecursiveFree(node->next); // Nest further
	free(node); // Free memory
}

static void Dirbrowse_SaveLastDirectory(void) {
	char *buf = malloc(256);
	int len = snprintf(buf, 256, "%s\n", cwd);
	FS_WriteFile("ux0:data/elevenmpv/lastdir.txt", buf, len);
	free(buf);
}

int Dirbrowse_PopulateFiles(SceBool refresh) {
	SceUID dir = 0;
	Dirbrowse_RecursiveFree(files);
	files = NULL;
	file_count = 0;

	if (R_SUCCEEDED(dir = sceIoDopen(cwd))) {
		int entryCount = 0, i = 0;
		SceIoDirent *entries = (SceIoDirent *)calloc(MAX_FILES, sizeof(SceIoDirent));

		while (sceIoDread(dir, &entries[entryCount]) > 0)
			entryCount++;

		sceIoDclose(dir);
		qsort(entries, entryCount, sizeof(SceIoDirent), Utils_Alphasort);

		for (i = 0; i < entryCount; i++) {
			// Ingore null filename
			if (entries[i].d_name[0] == '\0') 
				continue;

			// Ignore "." in all directories
			if (!strcmp(entries[i].d_name, ".")) 
				continue;

			// Ignore ".." in Root Directory
			if ((!strcmp(cwd, ROOT_PATH)) && (!strncmp(entries[i].d_name, "..", 2))) // Ignore ".." in Root Directory
				continue;

			// Allocate Memory
			File *item = (File *)malloc(sizeof(File));
			memset(item, 0, sizeof(File));

			item->is_dir = SCE_S_ISDIR(entries[i].d_stat.st_mode);

			// Copy File Name
			strcpy(item->name, entries[i].d_name);
			strcpy(item->ext, FS_GetFileExt(item->name));

			// New List
			if (files == NULL) 
				files = item;

			// Existing List
			else {
				File *list = files;
					
				while(list->next != NULL) 
					list = list->next;

				list->next = item;
			}

			file_count++;
		}

		free(entries);
	}
	else
		return dir;

	if (!refresh) {
		if (position >= file_count) 
			position = file_count - 1; // Keep index
	}
	else 
		position = 0; // Refresh position

	return 0;
}

void Dirbrowse_DisplayFiles(void) {
	vita2d_font_draw_text(font, 102, 40 + ((72 - vita2d_font_text_height(font, 25, cwd)) / 2) + 20, RGBA8(255, 255, 255, 255), 25, cwd);

	int i = 0, printed = 0;
	File *file = files; // Draw file list

	for(; file != NULL; file = file->next) {
		if (printed == FILES_PER_PAGE) // Limit the files per page
			break;

		if (position < FILES_PER_PAGE || i > (position - FILES_PER_PAGE)) {
			if (i == position)
				vita2d_draw_rectangle(0, 112 + (72 * printed), 960, 72, RGBA8(230, 230, 230, 255));

			if (file->is_dir)
				vita2d_draw_texture(icon_dir, 15, 117 + (72 * printed));
			else if ((!strncasecmp(file->ext, "flac", 4)) || (!strncasecmp(file->ext, "it", 2)) || (!strncasecmp(file->ext, "mod", 3)) || (!strncasecmp(file->ext, "mp3", 3))
				|| (!strncasecmp(file->ext, "ogg", 3)) || (!strncasecmp(file->ext, "s3m", 3))|| (!strncasecmp(file->ext, "wav", 3)) || (!strncasecmp(file->ext, "xm", 2)))
				vita2d_draw_texture(icon_audio, 15, 117 + (72 * printed));
			else
				vita2d_draw_texture(icon_file, 15, 117 + (72 * printed));

			if (strncmp(file->name, "..", 2) == 0)
				vita2d_font_draw_text(font, 102, 120 + (72 / 2) + (72 * printed), RGBA8(51, 51, 51, 255), 25, "Parent folder");
			else 
				vita2d_font_draw_text(font, 102, 120 + (72 / 2) + (72 * printed), RGBA8(51, 51, 51, 255), 25, file->name);

			printed++; // Increase printed counter
		}

		i++; // Increase counter
	}
}

File *Dirbrowse_GetFileIndex(int index) {
	int i = 0;
	File *file = files; // Find file Item
	
	for(; file != NULL && i != index; file = file->next)
		i++;

	return file; // Return file
}

void Dirbrowse_OpenFile(void) {
	char path[512];
	File *file = Dirbrowse_GetFileIndex(position);

	if (file == NULL)
		return;

	strcpy(path, cwd);
	strcpy(path + strlen(path), file->name);

	if (file->is_dir) {
		// Attempt to navigate to target
		if (R_SUCCEEDED(Dirbrowse_Navigate(SCE_FALSE))) {
			Dirbrowse_SaveLastDirectory();
			Dirbrowse_PopulateFiles(SCE_TRUE);
		}
	}
	else if ((!strncasecmp(file->ext, "flac", 4)) || (!strncasecmp(file->ext, "it", 2)) || (!strncasecmp(file->ext, "mod", 3)) || (!strncasecmp(file->ext, "mp3", 3)) 
		|| (!strncasecmp(file->ext, "ogg", 3)) || (!strncasecmp(file->ext, "s3m", 3))|| (!strncasecmp(file->ext, "wav", 3)) || (!strncasecmp(file->ext, "xm", 2)))
		Menu_PlayAudio(path);
}

// Navigate to Folder
int Dirbrowse_Navigate(SceBool parent) {
	File *file = Dirbrowse_GetFileIndex(position); // Get index
	
	if (file == NULL)
		return -1;

	// Special case ".."
	if ((parent) || (!strncmp(file->name, "..", 2))) {
		char *slash = NULL;

		// Find last '/' in working directory
		int i = strlen(cwd) - 2; for(; i >= 0; i--) {
			// Slash discovered
			if (cwd[i] == '/') {
				slash = cwd + i + 1; // Save pointer
				break; // Stop search
			}
		}

		slash[0] = 0; // Terminate working directory
	}

	// Normal folder
	else {
		if (file->is_dir) {
			// Append folder to working directory
			strcpy(cwd + strlen(cwd), file->name);
			cwd[strlen(cwd) + 1] = 0;
			cwd[strlen(cwd)] = '/';
		}
	}

	Dirbrowse_SaveLastDirectory();

	return 0; // Return success
}
