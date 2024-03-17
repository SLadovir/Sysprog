#include "userfs.h"
#include <stddef.h>

enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
	char *memory;
	/** How many bytes are occupied. */
	int occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;

	/* PUT HERE OTHER MEMBERS */
};

struct file {
	/** Double-linked list of file blocks. */
	struct block *block_list;
	/**
	 * Last block in the list above for fast access to the end
	 * of file.
	 */
	struct block *last_block;
	/** How many file descriptors are opened on the file. */
	int refs;
	/** File name. */
	char *name;
	/** Files are stored in a double-linked list. */
	struct file *next;
	struct file *prev;

	/* PUT HERE OTHER MEMBERS */
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;

	/* PUT HERE OTHER MEMBERS */
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

enum ufs_error_code
ufs_errno()
{
	return ufs_error_code;
}

int
ufs_open(const char *filename, int flags)
{
    // Поиск файла с указанным именем
    struct file *existing_file = file_list;
    while (existing_file != NULL) {
        if (strcmp(existing_file->name, filename) == 0) {
            // Файл найден

            // Если требуется только чтение
            if (flags == UFS_READ_ONLY) {
                existing_file->refs++;
                return existing_file->refs; // Возвращаем файловый дескриптор
            }

            // Если нужно создать новый файл и перезаписать существующий
            if (flags & UFS_CREATE) {
                ufs_delete(filename); // Удаляем существующий файл
                break;
            }

            // Если нужно только записать
            if (flags == UFS_WRITE_ONLY || flags == UFS_READ_WRITE) {
                ufs_delete(filename); // Удаляем существующий файл
                break;
            }
        }
        existing_file = existing_file->next;
    }

    // Создаем новый файл
    struct file *new_file = malloc(sizeof(struct file));
    if (new_file == NULL) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    new_file->block_list = NULL;
    new_file->last_block = NULL;
    new_file->refs = 1;
    new_file->name = strdup(filename); // Копируем имя файла

    // Добавляем новый файл в список файлов
    new_file->next = file_list;
    if (file_list != NULL) {
        file_list->prev = new_file;
    }
    file_list = new_file;

    // Создаем новый файловый дескриптор
    if (file_descriptor_count >= file_descriptor_capacity) {
        // Увеличиваем размер массива файловых дескрипторов
        file_descriptor_capacity += 10;
        file_descriptors = realloc(file_descriptors, sizeof(struct filedesc *) * file_descriptor_capacity);
        if (file_descriptors == NULL) {
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }
    }
    struct filedesc *new_filedesc = malloc(sizeof(struct filedesc));
    if (new_filedesc == NULL) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    new_filedesc->file = new_file;
    file_descriptors[file_descriptor_count] = new_filedesc;
    file_descriptor_count++;

    return file_descriptor_count; // Возвращаем файловый дескриптор
}

/*int
ufs_open_2(const char *filename, int flags)
{
    // Проверка наличия файла с указанным именем
    struct file *existing_file = file_list;
    while (existing_file != NULL) {
        if (strcmp(existing_file->name, filename) == 0) {
            // Файл найден

            // Проверка прав доступа к файлу
            if ((flags & UFS_READ_ONLY) && (existing_file->refs > 0 && (existing_file->refs & UFS_WRITE_ONLY || existing_file->refs & UFS_READ_WRITE))) {
                ufs_error_code = UFS_ERR_NO_PERMISSION;
                return -1;
            }
            if ((flags & UFS_WRITE_ONLY) && (existing_file->refs > 0 && (existing_file->refs & UFS_READ_ONLY || existing_file->refs & UFS_READ_WRITE))) {
                ufs_error_code = UFS_ERR_NO_PERMISSION;
                return -1;
            }

            // Если требуется только чтение
            if (flags & UFS_READ_ONLY) {
                existing_file->refs |= UFS_READ_ONLY;
                return existing_file->refs; // Возвращаем файловый дескриптор
            }

            // Если нужно только записать
            if (flags & UFS_WRITE_ONLY) {
                existing_file->refs |= UFS_WRITE_ONLY;
                return existing_file->refs; // Возвращаем файловый дескриптор
            }

            // Если нужно чтение и запись вместе
            existing_file->refs |= UFS_READ_WRITE;
            return existing_file->refs; // Возвращаем файловый дескриптор
        }
        existing_file = existing_file->next;
    }

    // Создание нового файла
    struct file *new_file = malloc(sizeof(struct file));
    if (new_file == NULL) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    new_file->block_list = NULL;
    new_file->last_block = NULL;
    new_file->refs = 0;
    new_file->name = strdup(filename); // Копирование имени файла

    // Добавление нового файла в список файлов
    new_file->next = file_list;
    if (file_list != NULL) {
        file_list->prev = new_file;
    }
    file_list = new_file;

    // Создание нового файлового дескриптора
    if (file_descriptor_count >= file_descriptor_capacity) {
        // Увеличение размера массива файловых дескрипторов
        file_descriptor_capacity += 10;
        file_descriptors = realloc(file_descriptors, sizeof(struct filedesc *) * file_descriptor_capacity);
        if (file_descriptors == NULL) {
            ufs_error_code = UFS_ERR_NO_MEM;
            return -1;
        }
    }
    struct filedesc *new_filedesc = malloc(sizeof(struct filedesc));
    if (new_filedesc == NULL) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    new_filedesc->file = new_file;
    file_descriptors[file_descriptor_count] = new_filedesc;
    file_descriptor_count++;

    // Устанавливаем флаги доступа в соответствии с переданными флагами
    if (flags & UFS_READ_ONLY) {
        new_file->refs |= UFS_READ_ONLY;
    }
    if (flags & UFS_WRITE_ONLY) {
        new_file->refs |= UFS_WRITE_ONLY;
    }
    if (!(flags & UFS_READ_ONLY) && !(flags & UFS_WRITE_ONLY)) {
        new_file->refs |= UFS_READ_WRITE;
    }

    return file_descriptor_count; // Возвращаем файловый дескриптор
}*/



ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
    if (fd < 1 || fd > file_descriptor_count || file_descriptors[fd - 1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct file *file = file_descriptors[fd - 1]->file;
    
    // Создаем новый блок данных для записи
    struct block *new_block = malloc(sizeof(struct block));
    if (new_block == NULL) {
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    new_block->memory = malloc(size);
    if (new_block->memory == NULL) {
        free(new_block);
        ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
    }
    memcpy(new_block->memory, buf, size);
    new_block->occupied = size;
    new_block->next = NULL;

    // Присоединяем новый блок к концу списка блоков файла
    if (file->block_list == NULL) {
        file->block_list = new_block;
    } else {
        file->last_block->next = new_block;
    }
    file->last_block = new_block;

    return size; // Возврат количества записанных байт
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
    if (fd < 1 || fd > file_descriptor_count || file_descriptors[fd - 1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct file *file = file_descriptors[fd - 1]->file;
    
    // Чтение данных из блоков файла
    ssize_t bytes_read = 0;
    struct block *current_block = file->block_list;
    while (current_block != NULL && bytes_read < size) {
        size_t bytes_to_copy = (size - bytes_read) < current_block->occupied ? (size - bytes_read) : current_block->occupied;
        memcpy(buf + bytes_read, current_block->memory, bytes_to_copy);
        bytes_read += bytes_to_copy;
        current_block = current_block->next;
    }

    return bytes_read; // Возврат количества прочитанных байт
}

int
ufs_close(int fd)
{
    if (fd < 1 || fd > file_descriptor_count || file_descriptors[fd - 1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct file *file = file_descriptors[fd - 1]->file;

    // Уменьшаем счетчик ссылок на файл
    file->refs--;

    // Если больше нет ссылок, освобождаем ресурсы
    if (file->refs == 0) {
        // Освобождаем блоки памяти файла
        struct block *current_block = file->block_list;
        while (current_block != NULL) {
            struct block *next_block = current_block->next;
            free(current_block->memory);
            free(current_block);
            current_block = next_block;
        }

        // Удаляем файл из списка файлов
        if (file->prev != NULL) {
            file->prev->next = file->next;
        }
        if (file->next != NULL) {
            file->next->prev = file->prev;
        }
        if (file == file_list) {
            file_list = file->next;
        }

        // Освобождаем имя файла
        free(file->name);

        // Освобождаем структуру файла
        free(file);
    }

    // Освобождаем файловый дескриптор
    free(file_descriptors[fd - 1]);
    file_descriptors[fd - 1] = NULL;

    return 0; // Успешное закрытие файла
}

int
ufs_delete(const char *filename)
{
    // Поиск файла с указанным именем
    struct file *file = file_list;
    while (file != NULL) {
        if (strcmp(file->name, filename) == 0) {
            break;
        }
        file = file->next;
    }

    if (file == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1; // Файл не найден
    }

    // Освобождаем блоки памяти файла
    struct block *current_block = file->block_list;
    while (current_block != NULL) {
        struct block *next_block = current_block->next;
        free(current_block->memory);
        free(current_block);
        current_block = next_block;
    }

    // Удаляем файл из списка файлов
    if (file->prev != NULL) {
        file->prev->next = file->next;
    }
    if (file->next != NULL) {
        file->next->prev = file->prev;
    }
    if (file == file_list) {
        file_list = file->next;
    }

    // Освобождаем имя файла
    free(file->name);

    // Освобождаем структуру файла
    free(file);

    return 0; // Успешное удаление файла
}

void
ufs_destroy(void)
{
}

#ifdef NEED_RESIZE
int
ufs_resize(int fd, size_t new_size)
{
    if (fd < 1 || fd > file_descriptor_count || file_descriptors[fd - 1] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
    struct file *file = file_descriptors[fd - 1]->file;

    // Проверяем, имеет ли пользователь право изменять размер файла
    if (!(file->refs == 1 && new_size <= MAX_FILE_SIZE)) {
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }

    // Определяем текущий размер файла
    size_t current_size = 0;
    struct block *current_block = file->block_list;
    while (current_block != NULL) {
        current_size += current_block->occupied;
        current_block = current_block->next;
    }

    if (new_size == current_size) {
        return 0; // Новый размер совпадает с текущим
    }

    if (new_size < current_size) {
        // Уменьшаем размер файла
        size_t bytes_to_remove = current_size - new_size;
        while (bytes_to_remove > 0 && file->last_block != NULL) {
            if (file->last_block->occupied > bytes_to_remove) {
                // Обрезаем блок, если его размер больше, чем необходимо удалить
                file->last_block->occupied -= bytes_to_remove;
                bytes_to_remove = 0;
            } else {
                // Удаляем целый блок
                struct block *prev_block = file->last_block->prev;
                free(file->last_block->memory);
                free(file->last_block);
                if (prev_block != NULL) {
                    prev_block->next = NULL;
                    file->last_block = prev_block;
                } else {
                    file->block_list = NULL;
                    file->last_block = NULL;
                }
                bytes_to_remove -= BLOCK_SIZE;
            }
        }
    } else {
        // Увеличиваем размер файла
        size_t bytes_to_add = new_size - current_size;
        while (bytes_to_add > 0) {
            struct block *new_block = malloc(sizeof(struct block));
            if (new_block == NULL) {
                // Недостаточно памяти
                ufs_error_code = UFS_ERR_NO_MEM;
                return -1;
            }
            new_block->memory = malloc(BLOCK_SIZE);
            if (new_block->memory == NULL) {
                // Недостаточно памяти
                free(new_block);
                ufs_error_code = UFS_ERR_NO_MEM;
                return -1;
            }
            new_block->occupied = (bytes_to_add > BLOCK_SIZE) ? BLOCK_SIZE : bytes_to_add;
            memset(new_block->memory, 0, BLOCK_SIZE);
            new_block->next = NULL;
            if (file->block_list == NULL) {
                file->block_list = new_block;
            } else {
                file->last_block->next = new_block;
            }
            file->last_block = new_block;
            bytes_to_add -= BLOCK_SIZE;
        }
    }

    return 0; // Успешное изменение размера файла
}
#endif
