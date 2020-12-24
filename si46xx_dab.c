/*
 * dabpi_ctl - raspberry pi fm/fmhd/dab receiver board control interface
 * Copyright (C) 2014  Bjoern Biesenbach <bjoern@bjoern-b.de>
 * Copyright (C) 2016  Heiko Jehmlich <hje@jecons.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "si46xx.h"

uint8_t dab_num_channels;

void si46xx_init_dab(void)
{
    si46xx_reset();
    si46xx_powerup();
    si46xx_hostload("firmware/rom00_patch_mini.bin");
    si46xx_hostload("firmware/rom00_patch.016.bin");
    si46xx_hostload("firmware/dab_radio.bin");

    si46xx_boot();

    si46xx_get_sys_state();
    si46xx_get_partinfo();

    //	si46xx_set_property(SI46XX_DAB_CTRL_DAB_MUTE_SIGNAL_LEVEL_THRESHOLD, 0);
    //	si46xx_set_property(SI46XX_DAB_CTRL_DAB_MUTE_SIGLOW_THRESHOLD, 0);
    //	si46xx_set_property(SI46XX_DAB_CTRL_DAB_MUTE_ENABLE, 0);
    //	si46xx_set_property(SI46XX_DAB_CTRL_DAB_ACF_ENABLE, 0x0000);
    si46xx_set_property(SI46XX_DIGITAL_SERVICE_INT_SOURCE, 0x0003); // enable DSRVPAKTINT interrupt ??
    si46xx_set_property(SI46XX_DAB_TUNE_FE_CFG, 0x0001);            // front end switch closed
    si46xx_set_property(SI46XX_DAB_TUNE_FE_VARM, 0x1710);           // Front End Varactor configuration (Changed from '10' to 0x1710 to improve receiver sensitivity - Bjoern 27.11.14)
    si46xx_set_property(SI46XX_DAB_TUNE_FE_VARB, 0x1711);           // Front End Varactor configuration (Changed from '10' to 0x1711 to improve receiver sensitivity - Bjoern 27.11.14)
    //si46xx_set_property(SI46XX_PIN_CONFIG_ENABLE, 0x8002);          // enable I2S output (since i2s isn't working on a raspberry pi we're gonna use analog audio )
    si46xx_set_property(SI46XX_PIN_CONFIG_ENABLE, 0x8001);          // enable analog audio output
    si46xx_set_property(SI46XX_DAB_INT_CTL_ENABLE, 0x0050);         // Enable DSRVINT
    si46xx_set_property(SI46XX_DAB_CRTL_XPAD_ENABLE, 0xBFDD);       //Enable all data types
    si46xx_set_property(SI46XX_DAB_DRC_OPTION, 0x0002);             //Enable 1/2 DRC gain
    //si46xx_set_property(SI46XX_DAB_ANNOUNCEMENT_ENABLE, 0x07FF); //Enable announcements
}

void si46xx_dab_digrad_status_print(struct dab_digrad_status_t *status)
{
    //printf("ACQ         : %d\r\n", status->acq);
    //printf("VALID       : %d\r\n", status->valid);
    //printf("RSSI        : %d\r\n", status->rssi);
    //printf("SNR         : %d\r\n", status->snr);
    //printf("FIC_QUALITY : %d\r\n", status->fic_quality);
    //printf("CNR         : %d\r\n", status->cnr);
    //printf("FFT_OFFSET  : %d\r\n", status->fft_offset);
    //printf("Tuned freq  : %d kHz\r\n", status->frequency);
    //printf("Tuned index : %d\r\n", status->tuned_index);
    //printf("ANTCAP      : %d\r\n", status->read_ant_cap);
    printf("{\"acq\": \"%d\", \"valid\": \"%d\", \"rssi\": \"%d\", \"snr\": \"%d\", \"ficQuality\": \"%d\", \"cnr\": \"%d\", \"fftOffset\": \"%d\", \"tunedFrequency\": \"%d\", \"tunedIndex\": \"%d\", \"antcap\": \"%d\"}", status->acq, status->valid, status->rssi, status->snr, status->fic_quality, status->cnr, status->fft_offset, status->frequency, status->tuned_index, status->read_ant_cap);
}

si46xx_swap_services(uint8_t first, uint8_t second)
{
    struct dab_service_t tmp;

    memcpy(&tmp, &dab_service_list.services[first], sizeof(tmp));
    memcpy(&dab_service_list.services[first], &dab_service_list.services[second], sizeof(tmp));
    memcpy(&dab_service_list.services[second], &tmp, sizeof(tmp));
}

si46xx_sort_service_list(void)
{
    uint8_t i, p, swapped;

    swapped = 0;
    for (i = dab_service_list.num_services; i > 1; i--)
    {
        for (p = 0; p < i - 1; p++)
        {
            if (dab_service_list.services[p].service_id > dab_service_list.services[p + 1].service_id)
            {
                si46xx_swap_services(p, p + 1);
                swapped = 1;
            }
        }
        if (!swapped)
            break;
    }
}

void si46xx_dab_print_service_list()
{
    uint8_t i, p;
    uint32_t service_id;
    uint16_t component_id;
    char *service_label;

    printf("List size:     %d\r\n", dab_service_list.list_size);
    printf("List version:  %d\r\n", dab_service_list.version);
    printf("Services:      %d\r\n", dab_service_list.num_services);
    printf("\r\n");
    printf(" Nr | Service ID | Service Name     | Component IDs\r\n");
    printf("--------------------------------------------------\r\n");
    for (i = 0; i < dab_service_list.num_services; i++)
    {
        service_id = dab_service_list.services[i].service_id;
        component_id = dab_service_list.services[i].component_id[0];
        service_label = dab_service_list.services[i].service_label;
        printf(" %02u |   %8x | %s | ", i, service_id, service_label, component_id);
        for (p = 0; p < dab_service_list.services[i].num_components; p++)
        {
            printf("%d ", dab_service_list.services[i].component_id[p]);
        }
        printf("\r\n");
    }
}

void si46xx_dab_parse_list(uint8_t *data, uint16_t len)
{
    uint16_t pos;
    uint8_t component_num;
    uint8_t i, p;

    if (len < 6)
        return; // no list available? exit
    if (len >= 9)
    {
        dab_service_list.list_size = data[5] << 8 | data[4];
        dab_service_list.version = data[7] << 8 | data[6];
        dab_service_list.num_services = data[8];
    }
    // 9,10,11 are align pad
    pos = 12;
    if (len <= pos)
        return; // no services? exit

    // size of one service with zero component: 24 byte
    // every component + 4 byte
    for (i = 0; i < dab_service_list.num_services; i++)
    {
        dab_service_list.services[i].service_id = data[pos + 3] << 24 | data[pos + 2] << 16 | data[pos + 1] << 8 | data[pos];
        component_num = data[pos + 5] & 0x0F;
        dab_service_list.services[i].num_components = component_num;
        memcpy(dab_service_list.services[i].service_label, &data[pos + 8], 16);
        dab_service_list.services[i].service_label[16] = '\0';
        for (p = 0; p < component_num; p++)
        {
            dab_service_list.services[i].component_id[p] = data[pos + 25] << 8 | data[pos + 24];
            pos += 4;
        }
        pos += 24;
    }
    si46xx_sort_service_list();
}

void si46xx_dab_set_freq_list(uint8_t num, uint32_t *freq_list)
{
    uint8_t data[3 + 4 * 48]; // max 48 frequencies
    uint8_t i;

    dab_num_channels = num;
    if (num == 0 || num > 48)
    {
        printf("num must be between 1 and 48\r\n");
        return;
    }

    data[0] = SI46XX_DAB_SET_FREQ_LIST;
    data[1] = num; // NUM_FREQS 1-48
    data[2] = 0;
    data[3] = 0;
    for (i = 0; i < num; i++)
    {
        data[4 + 4 * i] = freq_list[i] & 0xFF;
        data[5 + 4 * i] = freq_list[i] >> 8;
        data[6 + 4 * i] = freq_list[i] >> 16;
        data[7 + 4 * i] = freq_list[i] >> 24;
    }
    spi(data, 3 + 4 * num);
    si46xx_reply("DAB_SET_FREQ_LIST");
}

void si46xx_dab_tune_freq(uint8_t index, uint8_t antcap)
{
    uint8_t data[6];
    uint8_t timeout = 50;

    data[0] = SI46XX_DAB_TUNE_FREQ;
    data[1] = 0;
    data[2] = index;
    data[3] = 0;
    data[4] = antcap;
    data[5] = 0;

    while (timeout--)
    {
        spi(data, 6);
        if ((data[1] > 0x80)) // CTS and STC
        {
            hexDump("DAB_TUNE_FREQ", data, 5);
            return;
        }
        msleep(30);
    }
    printf("timeout on DAB_TUNE_FREQ\r\n");
}

int si46xx_dab_get_digital_service_list()
{
    uint8_t data[7];
    uint8_t timeout = 10;
    data[0] = SI46XX_DAB_GET_DIGITAL_SERVICE_LIST;
    data[1] = 0;

    uint16_t len;
    while (timeout--)
    {
        spi(data, 7);

        if (data[1] >= 0x80) // CTS
        {
            hexDump("DAB_GET_DIGITAL_SERVICE_LIST", data, 7);
            len = ((uint16_t)data[6] << 8) | (uint16_t)data[5];
            uint8_t buf[2047];
            buf[0] = 0;
            spi(buf, 7 + len);
            si46xx_dab_parse_list(buf + 1, len + 1);
            return len;
        } else {
            data[0] = 0;
        }
        msleep(20);
    }
    printf("timeout on %s\r\n", "DAB_GET_DIGITAL_SERVICE_LIST");

    //return len;
}

void si46xx_dab_start_digital_service_num(uint32_t num)
{
    uint8_t data[12];
    uint32_t service_id = dab_service_list.services[num].service_id;
    uint16_t component_id = dab_service_list.services[num].component_id[0];
    char *service_label = dab_service_list.services[num].service_label;

    data[0] = SI46XX_DAB_START_DIGITAL_SERVICE;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = service_id & 0xFF;
    data[5] = (service_id >> 8) & 0xFF;
    data[6] = (service_id >> 16) & 0xFF;
    data[7] = (service_id >> 24) & 0xFF;
    data[8] = component_id & 0xFF;
    data[9] = (component_id >> 8) & 0xFF;
    data[10] = (component_id >> 16) & 0xFF;
    data[11] = (component_id >> 24) & 0xFF;

    printf("Starting service %s %x %x\r\n", service_label, service_id, component_id);
    spi(data, 12);
    si46xx_reply("DAB_START_DIGITAL_SERVICE");
}

void si46xx_dab_start_digital_service_data(uint32_t num)
{
    uint8_t data[12];
    uint32_t service_id = dab_service_list.services[num].service_id;
    uint16_t component_id = dab_service_list.services[num].component_id[0];
    char *service_label = dab_service_list.services[num].service_label;

    data[0] = SI46XX_DAB_START_DIGITAL_SERVICE;
    data[1] = 0x0001;
    data[2] = 0;
    data[3] = 0;
    data[4] = service_id & 0xFF;
    data[5] = (service_id >> 8) & 0xFF;
    data[6] = (service_id >> 16) & 0xFF;
    data[7] = (service_id >> 24) & 0xFF;
    data[8] = component_id & 0xFF;
    data[9] = (component_id >> 8) & 0xFF;
    data[10] = (component_id >> 16) & 0xFF;
    data[11] = (component_id >> 24) & 0xFF;

    printf("Starting service %s %x %x\r\n", service_label, service_id, component_id);
    spi(data, 12);
    si46xx_reply("DAB_START_DIGITAL_SERVICE_DATA");
}
void si46xx_dab_stop_digital_service_data(uint32_t num)
{
    uint8_t data[12];
    uint32_t service_id = dab_service_list.services[num].service_id;
    uint16_t component_id = dab_service_list.services[num].component_id[0];
    char *service_label = dab_service_list.services[num].service_label;

    data[0] = SI46XX_DAB_STOP_DIGITAL_SERVICE;
    data[1] = 0x0001;
    data[2] = 0;
    data[3] = 0;
    data[4] = service_id & 0xFF;
    data[5] = (service_id >> 8) & 0xFF;
    data[6] = (service_id >> 16) & 0xFF;
    data[7] = (service_id >> 24) & 0xFF;
    data[8] = component_id & 0xFF;
    data[9] = (component_id >> 8) & 0xFF;
    data[10] = (component_id >> 16) & 0xFF;
    data[11] = (component_id >> 24) & 0xFF;

    printf("Starting service %s %x %x\r\n", service_label, service_id, component_id);
    spi(data, 12);
    si46xx_reply("DAB_START_DIGITAL_SERVICE_DATA");
}

void si46xx_dab_scan()
{
    uint8_t i;
    struct dab_digrad_status_t status;

    for (i = 0; i < dab_num_channels; i++)
    {
        si46xx_dab_tune_freq(i, 0);
        msleep(1000);
        si46xx_dab_digrad_status(&status);
        printf("Channel %d: ACQ: %d RSSI: %d SNR: %d ", i, status.acq, status.rssi, status.snr);
        if (status.acq)
        {
            msleep(1000);
            si46xx_dab_get_ensemble_info();
        };
        printf("\r\n");
    }
}

void si46xx_dab_digrad_status(struct dab_digrad_status_t *status)
{
    uint8_t data[23];
    uint8_t timeout = 10;

    while (timeout--)
    {
        data[0] = SI46XX_DAB_DIGRAD_STATUS;
        data[1] = (1 << 3) | 1; // set digrad_ack and stc_ack;
        spi(data, 2);
        msleep(10);
        data[0] = 0;
        spi(data, 23);
        if (data[1] & 0x81)
        {
            hexDump("DAB_DIGRAD_STATUS", data, 23);
            status->acq = (data[6] & 0x04) ? 1 : 0;
            status->valid = data[6] & 0x01;
            status->rssi = (int8_t)data[7];
            status->snr = (int8_t)data[8];
            status->fic_quality = data[9];
            status->cnr = data[10];
            status->frequency = data[13] | data[14] << 8 | data[15] << 16 | data[16] << 24;
            status->tuned_index = data[17];
            status->fft_offset = (int8_t)data[18];
            status->read_ant_cap = data[19] | data[20] << 8;
            return;
        }
        msleep(10);
    }
    printf("timeout on DAB_DIGRAD_STATUS\r\n");
}

void si46xx_dab_get_ensemble_info()
{
    char data[27];
    char label[17];
    uint8_t timeout = 10;

    data[0] = SI46XX_DAB_GET_ENSEMBLE_INFO;
    //data[1] = (1<<4) | (1<<0); // force_wb, low side injection
    data[1] = 0;
    spi(data, 2);

    while (timeout--)
    {
        data[0] = 0;
        spi(data, 27);
        if (data[1] & 0x80)
        {
            hexDump("DAB_GET_ENSEMBLE_INFO", data, 27);
            memcpy(label, data + 7, 16);
            label[16] = '\0';
            printf("Name: %s\r\n", label);
            printf("Ensemble ECC: %x\r\n", data[23]);
            printf("Charset: %x\r\n", data[24]);
            printf("abbrev1: %d\r\n", data[25]);
            printf("abbrev0: %d\r\n", data[26]);
            return;
        }
        msleep(10);
    }
    printf("timeout on DAB_GET_ENSEMBLE_INFO\r\n");
}

void si46xx_dab_get_audio_info(void)
{
    char data[11];
    uint8_t timeout = 10;

    data[0] = SI46XX_DAB_GET_AUDIO_INFO;
    data[1] = 0;
    spi(data, 2);
    
    FILE *stream;
    char *buf;
    size_t len;
    
    stream = open_memstream(&buf, &len);
    
    while (timeout--)
    {
        data[0] = 0;
        spi(data, 11);
        if (data[1] & 0x81)
        {
            hexDump("DAB_GET_AUDIO_INFO", data, 10);
            //printf("Bitrate    : %d kbps\r\n", data[5] + (data[6] << 8));
            fprintf(stream, "{\"bitrate\": \"%d\", ", data[5] + (data[6] << 8));
            //printf("Samplerate : %d Hz\r\n", data[7] + (data[8] << 8));
            fprintf(stream, "\"samplerate\": \"%d\", ", data[7] + (data[8] << 8));
            if ((data[9] & 0x03) == 0)
            {
                //printf("Audio Mode : Dual Mono\r\n");
                fprintf(stream, "\"audioMode\": \"dual\", ");
            }
            if ((data[9] & 0x03) == 1)
            {
                //printf("Audio Mode : Mono\r\n");
                fprintf(stream, "\"audioMode\": \"mono\", ");
            }
            if ((data[9] & 0x03) == 2)
            {
                //printf("Audio Mode : Stereo\r\n");
                fprintf(stream, "\"audioMode\": \"stereo\", ");
            }
            if ((data[9] & 0x03) == 3)
            {
                //printf("Audio Mode : Joint Stereo\r\n");
                fprintf(stream, "\"audioMode\": \"joint-stereo\", ");
            }
            //printf("SBR        : %d\r\n", (data[9] & 0x04) ? 1 : 0);
            //printf("PS         : %d\r\n", (data[9] & 0x08) ? 1 : 0);
            //printf("DRC GAIN   : %d\r\n", data[10]);
            fprintf(stream, "\"sbr\": \"%d\", \"ps\": \"%d\", \"drcGain\": \"%d\"}", (data[9] & 0x04) ? 1 : 0, (data[9] & 0x08) ? 1 : 0, data[10]);
            fclose(stream);
            printf(buf);
            free(buf);
            return;
        }
        msleep(10);
    }
    //printf("timeout on DAB_GET_AUDIO_INFO\r\n");
}

void si46xx_dab_get_time(void)
{
    char data[12];
    uint8_t timeout = 10;

    data[0] = SI46XX_DAB_GET_TIME;
    data[1] = 0;
    spi(data, 2);

    while (timeout--)
    {
        data[0] = 0;
        spi(data, 12);
        if (data[1] & 0x81)
        {
            hexDump("DAB_GET_TIME", data, 12);
            printf("Year    : %d\r\n", data[5] + (data[6] << 8));
            printf("Month   : %d \r\n", data[7]);
            printf("Days    : %d \r\n", data[8]);
            printf("Hours   : %d \r\n", data[9]);
            printf("Minutes : %d \r\n", data[10]);
            printf("Seconds : %i \r\n", data[11]);
            return;
        }
        msleep(10);
    }
    printf("timeout on DAB_GET_TIME\r\n");
}

void si46xx_dab_get_subchannel_info(uint32_t num)
{
    uint8_t data[13];
    uint32_t service_id = dab_service_list.services[num].service_id;
    uint16_t component_id = dab_service_list.services[num].component_id[0];
    char *service_label = dab_service_list.services[num].service_label;
    uint8_t timeout = 10;

    data[0] = SI46XX_DAB_GET_SUBCHAN_INFO;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = service_id & 0xFF;
    data[5] = (service_id >> 8) & 0xFF;
    data[6] = (service_id >> 16) & 0xFF;
    data[7] = (service_id >> 24) & 0xFF;
    data[8] = component_id & 0xFF;
    data[9] = (component_id >> 8) & 0xFF;
    data[10] = (component_id >> 16) & 0xFF;
    data[11] = (component_id >> 24) & 0xFF;
    spi(data, 12);

    while (timeout--)
    {
        data[0] = 0;
        spi(data, 13);
        if (data[1] & 0x80)
        {
            hexDump("DAB_GET_SUBCHAN_INFO", data, 13);
            if (data[5] == 0)
            {
                printf("Service Mode       : Audio Stream Service\r\n");
            }
            if (data[5] == 1)
            {
                printf("Service Mode       : Data Stream Service\r\n");
            }
            if (data[5] == 2)
            {
                printf("Service Mode       : FIDC Service\r\n");
            }
            if (data[5] == 3)
            {
                printf("Service Mode       : MSC Data Packet Service\r\n");
            }
            if (data[5] == 4)
            {
                printf("Service Mode       : DAB+\r\n");
            }
            if (data[5] == 5)
            {
                printf("Service Mode       : DAB\r\n");
            }
            if (data[5] == 6)
            {
                printf("Service Mode       : FIC Service\r\n");
            }
            if (data[5] == 7)
            {
                printf("Service Mode       : XPAD Data\r\n");
            }
            if (data[5] == 8)
            {
                printf("Service Mode       : No Media\r\n");
            }
            if (data[6] == 1)
            {
                printf("Protection Mode    : UEP-1\r\n");
            }
            if (data[6] == 2)
            {
                printf("Protection Mode    : UEP-2\r\n");
            }
            if (data[6] == 3)
            {
                printf("Protection Mode    : UEP-3\r\n");
            }
            if (data[6] == 4)
            {
                printf("Protection Mode    : UEP-4\r\n");
            }
            if (data[6] == 5)
            {
                printf("Protection Mode    : UEP-5\r\n");
            }
            if (data[6] == 6)
            {
                printf("Protection Mode    : EEP-1A\r\n");
            }
            if (data[6] == 7)
            {
                printf("Protection Mode    : EEP-2A\r\n");
            }
            if (data[6] == 8)
            {
                printf("Protection Mode    : EEP-3A\r\n");
            }
            if (data[6] == 9)
            {
                printf("Protection Mode    : EEP-4A\r\n");
            }
            if (data[6] == 10)
            {
                printf("Protection Mode    : EEP-1B\r\n");
            }
            if (data[6] == 11)
            {
                printf("Protection Mode    : EEP-2B\r\n");
            }
            if (data[6] == 12)
            {
                printf("Protection Mode    : EEP-3B\r\n");
            }
            if (data[6] == 13)
            {
                printf("Protection Mode    : EEP-4B\r\n");
            }
            printf("Subchannel Bitrate : %d kbps\r\n", data[7] + (data[8] << 8));
            printf("Capacity Units     : %d CU\r\n", data[9] + (data[10] << 8));
            printf("CU Starting Adress : %d\r\n", data[11] + (data[12] << 8));
            return;
        }
        msleep(10);
    }
    printf("timeout on DAB_GET_SUBCHAN_INFO\r\n");
}

void si46xx_dab_get_component_info(uint32_t num)
{
    uint8_t data[34];
    uint32_t service_id = dab_service_list.services[num].service_id;
    uint16_t component_id = dab_service_list.services[num].component_id[0];
    //char *service_label = dab_service_list.services[num].service_label;
    uint8_t timeout = 10;

    data[0] = SI46XX_DAB_GET_COMPONENT_INFO;
    data[1] = 0;
    data[2] = 0;
    data[3] = 0;
    data[4] = service_id & 0xFF;
    data[5] = (service_id >> 8) & 0xFF;
    data[6] = (service_id >> 16) & 0xFF;
    data[7] = (service_id >> 24) & 0xFF;
    data[8] = component_id & 0xFF;
    data[9] = (component_id >> 8) & 0xFF;
    data[10] = (component_id >> 16) & 0xFF;
    data[11] = (component_id >> 24) & 0xFF;

    //printf("Starting service %s %x %x\r\n", service_label, service_id, component_id);
    spi(data, 12);

    while (timeout--)
    {
        data[0] = 0;
        spi(data, 34);
        if (data[1] & 0x81)
        {
            hexDump("DAB_GET_COMPONENT_INFO", data, 34);
            printf("Global ID    : %d\r\n", data[5]);
            /*printf("Samplerate : %d Hz\r\n", data[7] + (data[8] << 8));
			if ((data[9] & 0x03) == 0) {
				printf("Audio Mode : Dual Mono\r\n");
			}
			if ((data[9] & 0x03) == 1) {
				printf("Audio Mode : Mono\r\n");
			}
			if ((data[9] & 0x03) == 2) {
				printf("Audio Mode : Stereo\r\n");
			}
			if ((data[9] & 0x03) == 3) {
				printf("Audio Mode : Joint Stereo\r\n");
			}*/
            //printf("SBR        : %d\r\n", (data[9] & 0x04) ? 1 : 0);
            //printf("PS         : %d\r\n", (data[9] & 0x08) ? 1 : 0);
            printf("udata 26 NUMUA   : %d\r\n", data[27]);
            printf("udata 27 LENUA   : %d\r\n", data[28]);
            printf("udata 28 UATYPE  : %d\r\n", data[29]);
            printf("udata 29 UATYPE  : %d\r\n", data[30]);
            printf("udata 30 UADATALEN  : %d\r\n", data[31]);
            printf("udata 31 UADATA0  : %d\r\n", data[32]);
            printf("udata 32 UADATAN  : %d\r\n", data[33]);
            //printf("udata 2   : %d\r\n", data[34]);
            si46xx_reply_len("DAB_GET_COMPONENT_INFO", data[28] + 33);
            return;
        }
        msleep(10);
    }
    //si46xx_reply_len("DAB_GET_COMPONENT_INFO", data[28]+ 33);
}

void si46xx_dab_get_digital_service_data(void)
{
    uint8_t data[25];
    //uint32_t service_id = dab_service_list.services[num].service_id;
    //uint16_t component_id = dab_service_list.services[num].component_id[0];
    //char *service_label = dab_service_list.services[num].service_label;
    uint8_t timeout = 10;

    data[0] = SI46XX_DAB_GET_DIGITAL_SERVICE_DATA;
    data[1] = 0x0001; //Acknowledge interrupts
                      //data[2] = 0;

    spi(data, 2);

    while (timeout--)
    {
        data[0] = 0;
        spi(data, 25);
        if (data[1] & 0x81)
        {
            hexDump("DAB_GET_DIGITAL_SERVICE_DATA", data, 25);
            printf("Resp4   : %d\r\n", data[5]);
            printf("buff count: %d\r\n", data[6]);
            printf("srv_state   : %d\r\n", data[7]);
            printf("data_src   : %d\r\n", (data[8] & 192) >> 6);
            printf("DSCTy   : %x\r\n", (data[8] & 63));
            printf("ServiceId   : %x%x%x%x\r\n", data[12], data[11], data[10], data[9]);
            printf("ComponentId   : %d%d%d%d\r\n", data[16], data[15], data[14], data[13]);
            printf("UATYPE   : %d\r\n", data[17]);
            printf("UATYPE   : %d\r\n", data[18]);
            printf("bytes   : %x%x\r\n", data[20], data[19]);
            //printf("bytes   : %d\r\n", data[20]);
            printf("segnum   : %d\r\n", data[21]);
            printf("segnum   : %d\r\n", data[22]);
            printf("numsegs   : %d\r\n", data[23]);
            printf("numsegs   : %d\r\n", data[24]);
            printf("payload0   : %d\r\n", data[25]);
            printf("payload1   : %d\r\n", data[26]);

            si46xx_reply_len("DAB_DIGITAL_SERVICE_DATA", (data[20] << 8 | data[19]) + 24);
            return;
        }
        msleep(10);
    }
    //si46xx_reply_len("DAB_GET_COMPONENT_INFO",data[19]);
}
