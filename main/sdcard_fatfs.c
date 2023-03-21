#include "sdcard.h"
#include "sdcard_vfs.h"

#include <logging.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>
  #include <diskio_impl.h>
  #include <diskio_sdmmc.h>
  #include <esp_vfs_fat.h>

  #define SDCARD_VFS_MAX_FILES 4

  BYTE sdcard_pdrv = FF_DRV_NOT_USED;
  FATFS *sdcard_fatfs;

  static const char *fresult_str(FRESULT result)
  {
    switch(result) {
      case FR_OK: return "Succeeded";
      case FR_DISK_ERR: return "A hard error occurred in the low level disk I/O layer";
      case FR_INT_ERR: return "Assertion failed";
      case FR_NOT_READY: return "The physical drive cannot work";
      case FR_NO_FILE: return "Could not find the file";
      case FR_NO_PATH: return "Could not find the path";
      case FR_INVALID_NAME: return "The path name format is invalid";
      case FR_DENIED: return "Access denied due to prohibited access or directory full";
      case FR_EXIST: return "Access denied due to prohibited access";
      case FR_INVALID_OBJECT: return "The file/directory object is invalid";
      case FR_WRITE_PROTECTED: return "The physical drive is write protected";
      case FR_INVALID_DRIVE: return "The logical drive number is invalid";
      case FR_NOT_ENABLED: return "The volume has no work area";
      case FR_NO_FILESYSTEM: return "There is no valid FAT volume";
      case FR_MKFS_ABORTED: return "The f_mkfs() aborted due to any problem";
      case FR_TIMEOUT: return "Could not get a grant to access the volume within defined period";
      case FR_LOCKED: return "The operation is rejected according to the file sharing policy";
      case FR_NOT_ENOUGH_CORE: return "LFN working buffer could not be allocated";
      case FR_TOO_MANY_OPEN_FILES: return "Number of open files > FF_FS_LOCK";
      case FR_INVALID_PARAMETER: return "Given parameter is invalid";
      default: return "???";
    }
  }

  static inline size_t fatfs_sector_size(FATFS *fs)
  {
      return fs->ssize;
  }

  static inline size_t fatfs_total_sectors(FATFS *fs)
  {
      return (fs->n_fatent - 2) * fs->csize;
  }

  static inline size_t fatfs_used_sectors(FATFS *fs, DWORD nclst)
  {
      return (fs->n_fatent - 2 - nclst) * fs->csize;
  }

  static inline size_t fatfs_free_sectors(FATFS *fs, DWORD nclst)
  {
      return nclst * fs->csize;
  }

  int sdcard_vfs_stat(const struct vfs_dev *dev, struct vfs_stat *stat)
  {
    char drv[8];
    FATFS *fatfs;
    DWORD nclst;
    FRESULT fres;

    if (snprintf(drv, sizeof(drv), "%u:", sdcard_pdrv) >= sizeof(drv)) {
      LOG_ERROR("drv overflow");
      return -1;
    }

    if ((fres = f_getfree(drv, &nclst, &fatfs))) {
      stat->mounted = false;

      LOG_ERROR("f_mount: %s", fresult_str(fres));

      return -1;
    } else {
      stat->mounted = true;
    }

    stat->sector_size = fatfs_sector_size(fatfs);
    stat->total_sectors = fatfs_total_sectors(fatfs);
    stat->used_sectors = fatfs_used_sectors(fatfs, nclst);
    stat->free_sectors = fatfs_free_sectors(fatfs, nclst);

    return 0;
  }

  const struct vfs_dev sdcard_vfs_dev = {
    .stat_func = sdcard_vfs_stat,
  };

  int mount_sdcard_fatfs(sdmmc_card_t *card)
  {
    char drv[8];
    esp_err_t err;
    FRESULT fres;

    // allocate sdmmc drive
    if ((err = ff_diskio_get_drive(&sdcard_pdrv))) {
      LOG_ERROR("ff_diskio_get_drive: %s", esp_err_to_name(err));
      return -1;
    }

    ff_diskio_register_sdmmc(sdcard_pdrv, card);

    if (snprintf(drv, sizeof(drv), "%u:", sdcard_pdrv) >= sizeof(drv)) {
      LOG_ERROR("drv overflow");
      return -1;
    }

    // mount vfs
    if ((err = esp_vfs_fat_register(SDCARD_VFS_PATH, drv, SDCARD_VFS_MAX_FILES, &sdcard_fatfs))) {
      LOG_ERROR("esp_vfs_fat_register: %s", esp_err_to_name(err));
      return -1;
    }

    LOG_INFO("mount sdcard FATFS at VFS %s", SDCARD_VFS_PATH);

    // mount fatfs
    if ((fres = f_mount(sdcard_fatfs, drv, 1))) {
      LOG_ERROR("f_mount: %s", fresult_str(fres));
      return -1;
    }

    return 0;
  }

  int unmount_sdcard_fatfs(sdmmc_card_t *card)
  {
    char drv[8];
    FRESULT fres;
    esp_err_t err;

    if (snprintf(drv, sizeof(drv), "%u:", sdcard_pdrv) >= sizeof(drv)) {
      LOG_ERROR("drv overflow");
      return -1;
    }

    // unmount fatfs
    if ((fres = f_mount(0, drv, 0))) {
      LOG_ERROR("f_mount: %s", fresult_str(fres));
      return -1;
    }

    // unregister fatfs -> sdmmc
    ff_diskio_unregister(sdcard_pdrv);

    sdcard_pdrv = FF_DRV_NOT_USED;

    // unmount vfs
    if ((err = esp_vfs_fat_unregister_path(SDCARD_VFS_PATH))) {
      LOG_ERROR("esp_vfs_fat_unregister_path: %s", esp_err_to_name(err));
      return -1;
    }

    LOG_INFO("unmounted sdcard FATFS at VFS %s", SDCARD_VFS_PATH);

    return 0;
  }
#endif
