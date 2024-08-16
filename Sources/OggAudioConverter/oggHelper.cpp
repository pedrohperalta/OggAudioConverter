#include "oggHelper.h"

#define READ 1024

// Constructor
oggHelper::oggHelper() {}

// Destructor
oggHelper::~oggHelper() {}

// Anonymous namespace for internal linkage
namespace {
    void ErrorReport(char* ErrMsg) {
        printf("\n%s\n", ErrMsg);
    }
}

// Encode function
_BOOL oggHelper::encode(const char* file_in, const char* file_out, EncodeSetting es, VorbisComment ivc) {
    _BOOL hresult = TRUE;
    WAVHEADER wfh;
    FILE* f_in = fopen(file_in, "rb");
    FILE* f_out = fopen(file_out, "wb");

    long cumulative_read = 0;
    long PCM_total_size = 0;

    if(!f_in) {
        sprintf(ErrMsg, "Couldn't open %s for read! Aborting!", file_in);
        ErrorReport(ErrMsg);
        hresult = FALSE;
        goto done;
    }

    if(!f_out) {
        sprintf(ErrMsg, "Couldn't open %s for write! Aborting!", file_out);
        ErrorReport(ErrMsg);
        hresult = FALSE;
        goto done;
    }

    // Set wav header
    fread(&wfh, sizeof(WAVHEADER), 1, f_in);

    fseek(f_in, 0, SEEK_END);
    PCM_total_size = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);
    cumulative_read += sizeof(WAVHEADER);

    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;
    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;
    int result;

    // Encode setup
    vorbis_info_init(&vi);
    switch(es.encode_mode) {
        case VBR:
            if(es.vbr_quality > 1) es.vbr_quality = 1;
            result = vorbis_encode_init_vbr(&vi, es.channel, wfh.sample_rate, es.vbr_quality);
            break;
        case ABR:
            if(es.max_abr_br != -1) {
                result = (vorbis_encode_setup_managed(&vi, es.channel, wfh.sample_rate, es.min_abr_br, es.abr_br, es.max_abr_br) ||
                          vorbis_encode_ctl(&vi, OV_ECTL_RATEMANAGE2_SET, NULL) || vorbis_encode_setup_init(&vi));
            } else {
                result = vorbis_encode_init(&vi, es.channel, wfh.sample_rate, -1, es.abr_br, -1);
            }
            break;
        case CBR:
            result = vorbis_encode_init(&vi, es.channel, wfh.sample_rate, es.cbr_br, es.cbr_br, es.cbr_br);
            break;
        default:
            sprintf(ErrMsg, "Error in selecting the proper encoding mode! Aborting!");
            ErrorReport(ErrMsg);
            hresult = FALSE;
            goto done;
    }

    if(result < 0) {
        sprintf(ErrMsg, "An error occurred while setting up libVorbis encode environment! Aborting!");
        ErrorReport(ErrMsg);
        hresult = FALSE;
        goto done;
    }

    vorbis_comment_init(&vc);
    vorbis_comment_add_tag(&vc, "ALBUM", ivc.ALBUM);
    vorbis_comment_add_tag(&vc, "ARTIST", ivc.ARTIST);
    vorbis_comment_add_tag(&vc, "CONTACT", ivc.CONTACT);
    vorbis_comment_add_tag(&vc, "COPYRIGHT", ivc.COPYRIGHT);
    vorbis_comment_add_tag(&vc, "DATE", ivc.DATE);
    vorbis_comment_add_tag(&vc, "DESCRIPTION", ivc.DESCRIPTION);
    vorbis_comment_add_tag(&vc, "GENRE", ivc.GENRE);
    vorbis_comment_add_tag(&vc, "ISRC", ivc.ISRC);
    vorbis_comment_add_tag(&vc, "LICENSE", ivc.LICENSE);
    vorbis_comment_add_tag(&vc, "LOCATION", ivc.LOCATION);
    vorbis_comment_add_tag(&vc, "ORGANISATION", ivc.ORGANISATION);
    vorbis_comment_add_tag(&vc, "PERFORMER", ivc.PERFORMER);
    vorbis_comment_add_tag(&vc, "TITLE", ivc.TITLE);
    vorbis_comment_add_tag(&vc, "TRACKNUMBER", ivc.TRACKNUMBER);
    vorbis_comment_add_tag(&vc, "VERSION", ivc.VERSION);

    vorbis_analysis_init(&vd, &vi);
    vorbis_block_init(&vd, &vb);

    srand(time(NULL));
    ogg_stream_init(&os, rand());

    {
        ogg_packet header;
        ogg_packet header_comm;
        ogg_packet header_code;

        vorbis_analysis_headerout(&vd, &vc, &header, &header_comm, &header_code);
        ogg_stream_packetin(&os, &header);
        ogg_stream_packetin(&os, &header_comm);
        ogg_stream_packetin(&os, &header_code);

        while(true) {
            int result = ogg_stream_flush(&os, &og);
            if(result == 0) break;
            fwrite(og.header, 1, og.header_len, f_out);
            fwrite(og.body, 1, og.body_len, f_out);
        }
    }

    signed char readbuffer[(READ * 4)];
    while(true) {
        long i;
        long bytes = fread(readbuffer, 1, (READ * 4), f_in);
        cumulative_read += bytes;

        if(bytes == 0) {
            vorbis_analysis_wrote(&vd, 0);
            break;
        } else {
            float** buffer = vorbis_analysis_buffer(&vd, READ);
            if(wfh.num_channels == 2 && es.channel == OH_Mono) {
                for(long i = 0; i < bytes/4; i++) {
                    buffer[0][i] = ((((readbuffer[i*4+1]<<8) | (0x00ff & (int)readbuffer[i*4])) / 32768.f) + (((readbuffer[i*4+3]<<8) | (0x00ff & (int)readbuffer[i*4+2])) / 32768.f)) * 0.5f;
                }
            } else if(wfh.num_channels == 2 && es.channel == OH_Stereo) {
                for(long i = 0; i < bytes/4; i++) {
                    buffer[0][i] = ((readbuffer[i*4+1]<<8) | (0x00ff & (int)readbuffer[i*4])) / 32768.f;
                    buffer[1][i] = ((readbuffer[i*4+3]<<8) | (0x00ff & (int)readbuffer[i*4+2])) / 32768.f;
                }
            } else if(wfh.num_channels == 1 && es.channel == OH_Stereo) {
                for(long i = 0; i < bytes/2; i++) {
                    float monoChl = ((readbuffer[i*2+1]<<8) | (0x00ff & (int)readbuffer[i*2])) / 32768.f;
                    buffer[0][i] = monoChl;
                    buffer[1][i] = monoChl;
                }
            } else if(wfh.num_channels == 1 && es.channel == OH_Mono) {
                for(long i = 0; i < bytes/2; i++) {
                    buffer[0][i] = ((readbuffer[i*2+1]<<8) | (0x00ff & (int)readbuffer[i*2])) / 32768.f;
                }
            } else {
                sprintf(ErrMsg, "The input channel mix is currently not supported! Aborting!");
                ErrorReport(ErrMsg);
                hresult = FALSE;
                goto done;
            }
            vorbis_analysis_wrote(&vd, i);
        }

        while(vorbis_analysis_blockout(&vd, &vb) == 1) {
            vorbis_analysis(&vb, NULL);
            vorbis_bitrate_addblock(&vb);
            while(vorbis_bitrate_flushpacket(&vd, &op)) {
                ogg_stream_packetin(&os, &op);
                while(true) {
                    int result = ogg_stream_pageout(&os, &og);
                    if(result == 0) break;
                    fwrite(og.header, 1, og.header_len, f_out);
                    fwrite(og.body, 1, og.body_len, f_out);
                    if(ogg_page_eos(&og)) break;
                }
            }
        }
    }

    ogg_stream_clear(&os);
    vorbis_block_clear(&vb);
    vorbis_dsp_clear(&vd);
    vorbis_comment_clear(&vc);
    vorbis_info_clear(&vi);
done:
    if(f_in != NULL) fclose(f_in);
    if(f_out != NULL) fclose(f_out);
    return hresult;
}

// Decode function
_BOOL oggHelper::decode(const char* file_in, const char* file_out) {
    _BOOL hresult = TRUE;
    wavHeader h;

    FILE* f_in = fopen(file_in, "rb");
    FILE* f_out = fopen(file_out, "wb");

    if(!f_in) {
        sprintf(ErrMsg, "Couldn't open %s for read! Aborting!", file_in);
        ErrorReport(ErrMsg);
        hresult = FALSE;
        if(f_in != NULL) fclose(f_in);
        return hresult;
    }

    if(!f_out) {
        sprintf(ErrMsg, "Couldn't open %s for write! Aborting!", file_out);
        ErrorReport(ErrMsg);
        hresult = FALSE;
        if(f_in != NULL) fclose(f_in);
        if(f_out != NULL) fclose(f_out);
        return hresult;
    }

    OggVorbis_File vf;
    if(ov_open_callbacks(f_in, &vf, NULL, 0, OV_CALLBACKS_NOCLOSE) < 0) {
        printf("%s\n", "Input does not appear to be an Ogg bitstream.");
        hresult = FALSE;
        fclose(f_in);
        fclose(f_out);
        return hresult;
    }

    vorbis_info *info = ov_info(&vf, -1);
    hresult = h.write_prelim_header(f_out, info->channels, info->rate);
    if (hresult != TRUE) {
        printf("%s\n", "can't write prelim header");
        fclose(f_in);
        fclose(f_out);
        return hresult;
    }

    long cumulative_read = 0;
    long ogg_total_size = 0;
    fseek(f_in, 0, SEEK_END);
    ogg_total_size = ftell(f_in);
    fseek(f_in, 0, SEEK_SET);

    ogg_int16_t convbuffer[4096];
    int convsize = 4096;

    long byteWritten = 0;
    ogg_sync_state oy;
    ogg_stream_state os;
    ogg_page og;
    ogg_packet op;

    vorbis_info vi;
    vorbis_comment vc;
    vorbis_dsp_state vd;
    vorbis_block vb;

    char *buffer;
    int bytes;

    ogg_sync_init(&oy);

    while(1) {
        int eos = 0;
        int i;

        buffer = ogg_sync_buffer(&oy, 4096);
        bytes = fread(buffer, 1, 4096, f_in);
        ogg_sync_wrote(&oy, bytes);

        cumulative_read += bytes;

        if(ogg_sync_pageout(&oy, &og) != 1) {
            if(bytes < 4096) break;

            sprintf(ErrMsg, "Input does not appear to be an Ogg bitstream! Aborting!");
            ErrorReport(ErrMsg);
            hresult = FALSE;
            goto done;
        }

        ogg_stream_init(&os, ogg_page_serialno(&og));

        vorbis_info_init(&vi);
        vorbis_comment_init(&vc);

        if(ogg_stream_pagein(&os, &og) < 0) {
            sprintf(ErrMsg, "Error reading first page of Ogg bitstream data! Aborting!");
            ErrorReport(ErrMsg);
            hresult = FALSE;
            goto done;
        }

        if(ogg_stream_packetout(&os, &op) != 1) {
            sprintf(ErrMsg, "Error reading initial header packet! Aborting!");
            ErrorReport(ErrMsg);
            hresult = FALSE;
            goto done;
        }

        if(vorbis_synthesis_headerin(&vi, &vc, &op) < 0) {
            sprintf(ErrMsg, "This Ogg bitstream does not contain Vorbis audio data! Aborting!");
            ErrorReport(ErrMsg);
            hresult = FALSE;
            goto done;
        }

        i = 0;
        while(i < 2) {
            while(i < 2) {
                int result = ogg_sync_pageout(&oy, &og);
                if(result == 0) break;

                if(result == 1) {
                    ogg_stream_pagein(&os, &og);
                    while(i < 2) {
                        result = ogg_stream_packetout(&os, &op);
                        if(result == 0) break;
                        if(result < 0) {
                            sprintf(ErrMsg, "Corrupt secondary header! Aborting!");
                            ErrorReport(ErrMsg);
                            hresult = FALSE;
                            goto done;
                        }
                        result = vorbis_synthesis_headerin(&vi, &vc, &op);

                        if(result < 0) {
                            sprintf(ErrMsg, "Corrupt secondary header! Aborting!");
                            ErrorReport(ErrMsg);
                            hresult = FALSE;
                            goto done;
                        }
                        i++;
                    }
                }
            }

            buffer = ogg_sync_buffer(&oy, 4096);
            bytes = fread(buffer, 1, 4096, f_in);

            cumulative_read += bytes;

            if(bytes == 0 && i < 2) {
                sprintf(ErrMsg, "End of file before finding all Vorbis headers! Aborting!");
                ErrorReport(ErrMsg);
                hresult = FALSE;
                goto done;
            }
            ogg_sync_wrote(&oy, bytes);
        }

        convsize = 4096 / vi.channels;

        if(vorbis_synthesis_init(&vd, &vi) == 0) {
            vorbis_block_init(&vd, &vb);

            while(!eos) {
                while(!eos) {
                    int result = ogg_sync_pageout(&oy, &og);
                    if(result == 0) break;

                    if(result < 0) {
                        sprintf(ErrMsg, "Corrupt or missing data in bitstream! Continuing...");
                        ErrorReport(ErrMsg);
                    } else {
                        ogg_stream_pagein(&os, &og);

                        while(1) {
                            result = ogg_stream_packetout(&os, &op);
                            if(result == 0) break;
                            if(result < 0) {}

                            else {
                                float **pcm;
                                int samples;
                                if(vorbis_synthesis(&vb, &op) == 0)
                                    vorbis_synthesis_blockin(&vd, &vb);

                                while((samples = vorbis_synthesis_pcmout(&vd, &pcm)) > 0) {
                                    int j;
                                    int bout = (samples < convsize ? samples : convsize);

                                    for(i = 0; i < vi.channels; i++) {
                                        ogg_int16_t *ptr = convbuffer + i;
                                        float *mono = pcm[i];
                                        for(j = 0; j < bout; j++) {
                                            int val = floor(mono[j] * 32767.f + .5f);

                                            if(val > 32767) val = 32767;
                                            if(val < -32768) val = -32768;
                                            *ptr = val;
                                            ptr += vi.channels;
                                        }
                                    }

                                    int written = fwrite(convbuffer, 2 * vi.channels, bout, f_out);
                                    byteWritten += (written * 2 * vi.channels);

                                    vorbis_synthesis_read(&vd, bout);
                                }
                            }
                        }
                        if(ogg_page_eos(&og)) eos = 1;
                    }
                }

                if(!eos) {
                    buffer = ogg_sync_buffer(&oy, 4096);
                    bytes = fread(buffer, 1, 4096, f_in);

                    cumulative_read += bytes;

                    ogg_sync_wrote(&oy, bytes);
                    if(bytes == 0) eos = 1;
                }
            }

            vorbis_block_clear(&vb);
            vorbis_dsp_clear(&vd);

            hresult = h.rewrite_header(f_out, byteWritten);
            if (hresult != TRUE) {
                printf("%s\n", "can't rewrite header");
                fclose(f_out);
                fclose(f_in);
                return hresult;
            }
        } else {
            sprintf(ErrMsg, "Error: Corrupt header during playback initialization! Aborting!");
            ErrorReport(ErrMsg);
            hresult = FALSE;
        }

        ogg_stream_clear(&os);
        vorbis_comment_clear(&vc);
        vorbis_info_clear(&vi);
    }

    ogg_sync_clear(&oy);

done:
    if(f_in != NULL) fclose(f_in);
    if(f_out != NULL) fclose(f_out);

    return hresult;
}
