/* 
 * File:   spi_via_usi_driver.h
 * Author: tomd
 *
 * Created on 21 April 2013, 8:28 PM
 */

#ifndef SPI_VIA_USI_DRIVER_H
#define	SPI_VIA_USI_DRIVER_H

#ifdef	__cplusplus
extern "C" {
#endif
void spiX_initmaster( uint8_t spi_mode );
void spiX_initslave( uint8_t spi_mode );
uint8_t spiX_put( uint8_t val );
uint8_t spiX_get();
void spiX_wait();
uint8_t spiX_is_complete();




#ifdef	__cplusplus
}
#endif

#endif	/* SPI_VIA_USI_DRIVER_H */

