#ifndef _STOCK_H_
#define _STOCK_H_

/**************************************************************************/
/** @name Stock images                                                    */
/**************************************************************************/
/*@{*/
#define LSHW_STOCK_BLUETOOTH       "lshw-bluetooth"
#define LSHW_STOCK_DISC            "lshw-disc"
#define LSHW_STOCK_FIREWIRE        "lshw-firewire"
#define LSHW_STOCK_MODEM           "lshw-modem"
#define LSHW_STOCK_NETWORK         "lshw-network"
#define LSHW_STOCK_PRINTER         "lshw-printer"
#define LSHW_STOCK_RADIO           "lshw-radio"
#define LSHW_STOCK_SCSI            "lshw-scsi"
#define LSHW_STOCK_SERIAL          "lshw-serial"
#define LSHW_STOCK_USB             "lshw-usb"
#define LSHW_STOCK_WIFI            "lshw-wifi"
/*@}*/

/**
 * For getting the icon size for the logo
 */
#define LSHW_ICON_SIZE_LOGO        "lshw-icon-size-logo"

/**
 * Sets up the lshw stock repository.
 */
void lshw_gtk_stock_init(void);

#endif /* _STOCK_H_ */
