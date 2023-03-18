#include "sdcard.h"

#include <logging.h>

#if CONFIG_SDCARD_ENABLED
  #include <driver/sdmmc_types.h>
  #include <diskio_impl.h>
  #include <diskio_sdmmc.h>
  #include <esp_vfs_fat.h>

  #define SDCARD_FATFS_BASE_PATH "/sdcard"
  #define SDCARD_FATFS_MAX_FILES 4

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

  int mount_sdcard_fatfs(sdmmc_card_t *card)
  {
    BYTE pdrv = FF_DRV_NOT_USED;
    char drv[8];
    esp_err_t err;
    FRESULT fres;

    // allocate sdmmc drive
    if ((err = ff_diskio_get_drive(&pdrv))) {
      LOG_ERROR("ff_diskio_get_drive: %s", esp_err_to_name(err));
      return -1;
    }

    ff_diskio_register_sdmmc(pdrv, card);

    if (snprintf(drv, sizeof(drv), "%u:", pdrv) >= sizeof(drv)) {
      LOG_ERROR("drv overflow");
      return -1;
    }

    // mount vfs
    if ((err = esp_vfs_fat_register(SDCARD_FATFS_BASE_PATH, drv, SDCARD_FATFS_MAX_FILES, &sdcard_fatfs))) {
      LOG_ERROR("esp_vfs_fat_register: %s", esp_err_to_name(err));
      return -1;
    }

    LOG_INFO("mount sdcard FATFS at VFS %s", SDCARD_FATFS_BASE_PATH);

    // mount fatfs
    if ((fres = f_mount(sdcard_fatfs, drv, 1))) {
      LOG_ERROR("f_mount: %s", fresult_str(fres));
      return -1;
    }

    return 0;
  }

  int unmount_sdcard_fatfs(sdmmc_card_t *card)
  {
    BYTE pdrv = FF_DRV_NOT_USED;
    char drv[8];
    FRESULT fres;
    esp_err_t err;

    // lookup fatfs drive number
    if ((pdrv = ff_diskio_get_pdrv_card(card)) == FF_DRV_NOT_USED) {
      LOG_ERROR("ff_diskio_get_pdrv_card(...): not used");
      return -1;
    }

    if (snprintf(drv, sizeof(drv), "%u:", pdrv) >= sizeof(drv)) {
      LOG_ERROR("drv overflow");
      return -1;
    }

    // unmount fatfs
    if ((fres = f_mount(0, drv, 0))) {
      LOG_ERROR("f_mount: %s", fresult_str(fres));
      return -1;
    }

    // unregister fatfs -> sdmmc
    ff_diskio_unregister(pdrv);

    // unmount vfs
    if ((err = esp_vfs_fat_unregister_path(SDCARD_FATFS_BASE_PATH))) {
      LOG_ERROR("esp_vfs_fat_unregister_path: %s", esp_err_to_name(err));
      return -1;
    }

    LOG_INFO("unmounted sdcard FATFS at VFS %s", SDCARD_FATFS_BASE_PATH);

    return 0;
  }
#endif
