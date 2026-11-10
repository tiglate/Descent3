/*
* Descent 3
* Copyright (C) 2024 Parallax Software
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

--- HISTORICAL COMMENTS FOLLOW ---

 * $Logfile: /DescentIII/Main/dd_sndlib/ddsoundload.cpp $
 * $Revision: 17 $
 * $Date: 3/20/00 12:23p $
 * $Author: Matt $
 *
 *
 *
 * $Log: /DescentIII/Main/dd_sndlib/ddsoundload.cpp $
 *
 * 17    3/20/00 12:23p Matt
 * Merge of Duane's post-1.3 changes.
 * Fixed bug with sounds not being deleted
 *
 * 16    10/23/99 5:12p 3dsmax
 * use GlobalAlloc for win32 instead of mem_malloc for now (restored
 * original code)
 *
 * 15    10/22/99 8:10a Kevin
 * Fixed bug introduced from the mac merge
 *
 * 14    10/21/99 9:27p Jeff
 * B.A. Macintosh code merge
 *
 * 13    4/17/99 1:53a Jeff
 * made platform 'independent' so other low-level sound libs can include
 * without having to double code
 *
 * 12    4/06/99 8:29p Samir
 * added error check system.
 *
 * 11    2/16/99 12:42p Kevin
 * Improvements to the paging data progress indicator
 *
 * 10    2/09/99 7:01p Kevin
 * First work for new and improved progress screen while loading a level.
 * Note that this is a hack at this point, while I get the details worked
 * out, then I'll make it cleaner.
 *
 * 9     1/14/99 6:10p Samir
 * added DirectSound buffer duplication code.
 *
 * 8     10/22/98 8:31p Chris
 * Sounds use GlobalAlloc and GlobalFree
 *
 * 7     10/19/98 11:57a Chris
 * Update the sound system to use the import volume
 *
 * 6     10/12/98 1:27p Jeff
 * all mem_free's do set the value to NULL after they are complete
 *
 * 5     10/12/98 11:30a Kevin
 * More memory stuff
 *
 * 4     10/08/98 1:18a Chris
 * Now it is non-Intel platform friendly.
 *
 * 3     10/08/98 1:09a Chris
 * Speed up the sound paging in. -- Fucks up if byte ordering if different
 * then for the Intel platform.
 *
 * 2     7/24/98 5:23p Samir
 * moved sound variables from soundload to ssl_lib.h (and WAV file stuff)
 *
 * 1     7/20/98 2:54p Samir
 * moved code from sndlib.
 *
 * $NoKeywords: $
 */

#include <cstdlib>
#include <cstring>
#include <memory>
#include <limits>

#include "ssl_lib.h"
#include "cfile.h"
#include "mem.h"
#include "byteswap.h"
#include "gamesequence.h"
#include "log.h"

sound_info Sounds[MAX_SOUNDS];
sound_file_info SoundFiles[MAX_SOUND_FILES];

#define CLIP_ATTENUATION .5f

#define SOUND_FILE_SAMPLE_ALIGNMENT 4

#define SND_LOAD_ERROR_NO_FILE 0
#define SND_LOAD_ERROR_INVALID 1

namespace {

struct FreeSoundSamples {
  void operator()(void *samples) const { mem_free(samples); }
};

template <typename T> bool ReadWaveValue(CFILE *file, T &value) {
  T raw{};
  if (cf_ReadBytes(reinterpret_cast<uint8_t *>(&raw), sizeof(raw), file) != sizeof(raw))
    return false;
  value = D3::convert_le(raw);
  return true;
}

} // namespace

char SoundLoadWaveFile(const char *filename, float percent_volume, int sound_file_index, bool f_high_quality,
                       bool f_load_sample_data, int *e_type) {

  // File pointer to sound file
  CFILE *cfptr;
  std::unique_ptr<CFILE, decltype(&cfclose)> file(nullptr, cfclose);

  std::unique_ptr<uint8_t[], FreeSoundSamples> sample_8bit;
  std::unique_ptr<int16_t[], FreeSoundSamples> sample_16bit;
  int sample_length = 0;
  int np_sample_length = 0;

  char format_type[80]; // ASCII name of format type
  uint16_t fmttag = 0;  // Numerical format type
  uint32_t ckid;        // Current chunk's ID
  uint32_t cksize;      // Current chunk's size in bytes
  uint32_t actual_size;
  uint32_t filesize;     // Size of the sound file
  uint32_t nextseek = 0; // Location of the next seek

  uint32_t aligned_size; // Sound files are aligned to SOUND_FILE_SAMPLE_ALIGNMENT samples

  // Sound format information
  int samples_per_second;
  int16_t bits_per_sample;
  int16_t block_alignment;
  int16_t number_channels;

  // Used to read temporary long values
  uint32_t temp_long;

  // Tracks whether a format chunk precedes the data chunk.
  char f_fmt = 0;

  // Loop counter
  int count;

  // If there is an error assume it would be because of an invalid file
  if (e_type)
    *e_type = SND_LOAD_ERROR_INVALID;

  // Open the wave file
  cfptr = cfopen(filename, "rb");
  if (!cfptr) {
    if (e_type)
      *e_type = SND_LOAD_ERROR_NO_FILE;

    LOG_WARNING.printf("SOUND LOADER: %s not found", filename);
    goto error_state;
  }

  file.reset(cfptr);
  if (!f_load_sample_data) {
    return 1;
  }
  actual_size = cfilelength(cfptr);
  if (actual_size < 12 || actual_size > static_cast<uint32_t>(std::numeric_limits<int>::max()))
    goto error_state;

  // Used for progress bar when loading the level
  paged_in_count += static_cast<int>(actual_size);
  paged_in_num++;
  // Make sure that it is a RIFF format
  if (!ReadWaveValue(cfptr, temp_long))
    goto error_state;
  if (temp_long != 0x46464952) {
    LOG_WARNING.printf("SOUND LOADER: %s is not a RIFF format file", filename);
    goto error_state;
  }

  // The first item is the filesize minus the RIFF type specifier and filesize (add current location for total filesize)
  if (!ReadWaveValue(cfptr, filesize) || filesize < 4 || filesize > actual_size - 8)
    goto error_state;
  filesize += 8;

  // Make sure it is a wave file
  if (!ReadWaveValue(cfptr, temp_long))
    goto error_state;
  if (temp_long != 0x45564157) {
    LOG_WARNING.printf("SOUND LOADER:  %s is not a WAVE file", filename);
    goto error_state;
  }
  nextseek = cftell(cfptr);

  // Read in the chunks
  while (nextseek < filesize) {
    // Move the the beginning of the next chunck
    if (filesize - nextseek < 8 || cfseek(cfptr, nextseek, SEEK_SET) != 0)
      goto error_state;

    // Read in the chunk size and type
    if (!ReadWaveValue(cfptr, ckid) || !ReadWaveValue(cfptr, cksize))
      goto error_state;
    if (cksize == 0 || cksize > filesize - static_cast<uint32_t>(cftell(cfptr))) {
      LOG_WARNING.printf("SOUND LOADER: %s has an invalid block length", filename);
      goto error_state;
    }

    // Determine where the next chunk begins
    nextseek = cksize + static_cast<uint32_t>(cftell(cfptr));
    // Skip RIFF word padding, tolerating a missing pad on the final chunk.
    if ((cksize & 1) && nextseek < filesize)
      ++nextseek;

    // Determine what to do with the chunk
    switch (ckid) {
    case 0x20746D66: // Format Chunk
      if (cksize < 16)
        goto error_state;
      // Make sure that this format was not preceeded by another format without data inbetween them.
      if (f_fmt) {
        LOG_WARNING.printf("Sound Loader: Found 2 formats in a row in file %s", filename);
        goto error_state;
      }

      // Read in the format type
      if (!ReadWaveValue(cfptr, fmttag))
        goto error_state;

      switch (fmttag) {
      // We only support WAVE_FORMAT_PCM currently
      default:
      case 0x0000:
        strcpy(format_type, "WAVE_FORMAT_UNKNOWN");
        break;
      case 0x0001:
        strcpy(format_type, "WAVE_FORMAT_PCM");
        break;
      case 0x0002:
        strcpy(format_type, "WAVE_FORMAT_ADPCM");
        break;
      case 0x0005:
        strcpy(format_type, "WAVE_FORMAT_IBM_CVSD");
        break;
      case 0x0006:
        strcpy(format_type, "WAVE_FORMAT_ALAW");
        break;
      case 0x0007:
        strcpy(format_type, "WAVE_FORMAT_MULAW");
        break;
      case 0x0010:
        strcpy(format_type, "WAVE_FORMAT_OKI_ADPCM");
        break;
      case 0x0011:
        strcpy(format_type, "WAVE_FORMAT_DVI_ADPCM");
        break;
      case 0x0015:
        strcpy(format_type, "WAVE_FORMAT_DIGISTD");
        break;
      case 0x0016:
        strcpy(format_type, "WAVE_FORMAT_DIGIFIX");
        break;
      case 0x0020:
        strcpy(format_type, "WAVE_FORMAT_YAMAHA_ADPCM");
        break;
      case 0x0021:
        strcpy(format_type, "WAVE_FORMAT_SONARC");
        break;
      case 0x0022:
        strcpy(format_type, "WAVE_FORMAT_DSPGROUP_TRUESPEECH");
        break;
      case 0x0023:
        strcpy(format_type, "WAVE_FORMAT_ECHOSC1");
        break;
      case 0x0024:
        strcpy(format_type, "WAVE_FORMAT_AUDIOFILE_AF18");
        break;
      case 0x0101:
        strcpy(format_type, "IBM_FORMAT_MULAW");
        break;
      case 0x0102:
        strcpy(format_type, "IBM_FORMAT_ALAW");
        break;
      case 0x0103:
        strcpy(format_type, "IBM_FORMAT_ADPCM");
        break;
      case 0x0200:
        strcpy(format_type, "WAVE_FORMAT_CREATIVE_ADPCM");
        break;
      }

      // Currently, we only support PCM wave files
      if (fmttag != 0x0001) {
        LOG_WARNING.printf("SOUND LOADER: %s is a type %s wavefile, we only support WAVE_FORMAT_PCM waves.", filename,
                           format_type);
        goto error_state;
      }

      // Get the number of channels
      if (!ReadWaveValue(cfptr, number_channels))
        goto error_state;
      if (number_channels != 1) {
        LOG_WARNING.printf("SOUND LOADER: Invalid number of channels (%d) in %s, we want mono samples only.",
                           number_channels, filename);
        goto error_state;
      }

      // Get the number of samples per second
      if (!ReadWaveValue(cfptr, samples_per_second))
        goto error_state;
      if (samples_per_second != 22050) {
        LOG_WARNING.printf("SOUND LOADER: Invalid sample per second (%d) in %s, we want 22k samples only.",
                           samples_per_second, filename);
        goto error_state;
      }

      // get the Average bytes per second (ignore it)
      if (!ReadWaveValue(cfptr, temp_long))
        goto error_state;

      // Get the Block Alignment (ignore it)
      if (!ReadWaveValue(cfptr, block_alignment))
        goto error_state;

      // Get the Bits per sample
      if (!ReadWaveValue(cfptr, bits_per_sample))
        goto error_state;
      if (bits_per_sample != 8 && bits_per_sample != 16) {
        LOG_WARNING.printf("SOUND LOADER: Invalid bits per sample (%d) in %s, we want 8 or 16 bit samples only.",
                           bits_per_sample, filename);
        goto error_state;
      }

      // Beginning of some support for other wave file formats
      // if(fmttag!=0x0001)
      // {
      //      fread(&ra,1,2,fp);
      //      printf("  %u Bytes of extra information\n");
      //      printf("  ***I have not seen a file like this before, hope it goes well***\n");
      //      switch(fmttag)
      //      {
      //          case 0x0002:    //MS ADPCM?
      //              fread(&ra,1,2,fp);
      //              printf("  %u Samples Per Block\n",ra);
      //              fread(&ra,1,2,fp);
      //              printf("  %u Coefficients\n",ra);
      //              for(la=0;la<ra;la++)
      //              {
      //                  fread(&rb,1,2,fp);
      //                  fread(&rc,1,2,fp);
      //                  printf("    %u  %u\n",rb,rc);
      //              }
      //              break;
      //          case 0x0011: //WAVE_FORMAT_DVI_ADPCM
      //          case 0x0022: //WAVE_FORMAT_DSPGROUP_TRUESPEECH
      //              fread(&ra,1,2,fp);
      //              printf("  %u Samples Per Block\n",ra);
      //              break;
      //          case 0x0021:  // WAVE_FORMAT_SONARC
      //              fread(&ra,1,2,fp);
      //              printf("  %u CompType\n",ra);
      //              break;
      //          case 0x0200: //WAVE_FORMAT_CREATIVE_ADPCM
      //              fread(&ra,1,2,fp);
      //              printf("  Revision %u\n",ra);
      //              break;
      //      }
      //  }

      // We did find a new format type
      f_fmt = 1;
      break;

    // Data Chunk
    case 0x61746164: {
      if (!f_fmt) {
        LOG_WARNING.printf("SOUND LOADER: Format Chunk not defined before Data Chunk");
        goto error_state;
      }

      if ((bits_per_sample == 16 && (cksize & 1)) ||
          cksize > static_cast<uint32_t>(std::numeric_limits<int>::max()) - 8)
        goto error_state;
      // The bounds above leave room for alignment in the legacy int-sized APIs.
      const int data_size = static_cast<int>(cksize);

      // Preserve last-waveform-wins semantics without retaining earlier storage,
      // including when successive waveforms have different bit depths.
      sample_8bit.reset();
      sample_16bit.reset();

      // It is either 8 or 16 bit as validated from above
      if (bits_per_sample == 8) {
        int num_needed = 0;

        // Force sample alignment
        if ((cksize - 1) % SOUND_FILE_SAMPLE_ALIGNMENT) {
          num_needed = SOUND_FILE_SAMPLE_ALIGNMENT - ((cksize - 1) % SOUND_FILE_SAMPLE_ALIGNMENT);
          aligned_size = cksize + num_needed;
        } else {
          aligned_size = cksize;
        }

        sample_length = static_cast<int>(aligned_size);
        np_sample_length = data_size;

        sample_8bit.reset(mem_rmalloc<uint8_t>(aligned_size));
        if (!sample_8bit)
          goto error_state;

        if (cf_ReadBytes(sample_8bit.get(), data_size, cfptr) != data_size)
          goto error_state;

        if (aligned_size != cksize)
          memset(sample_8bit.get() + cksize, 128, num_needed);
      } else {
        int num_needed = 0;

        sample_length = data_size / 2;
        np_sample_length = data_size / 2;

        if ((sample_length - 1) % SOUND_FILE_SAMPLE_ALIGNMENT) {
          num_needed = (SOUND_FILE_SAMPLE_ALIGNMENT - ((sample_length - 1) % SOUND_FILE_SAMPLE_ALIGNMENT)) * 2;
          sample_length = data_size / 2 + num_needed / 2;
        }

        sample_16bit.reset(mem_rmalloc<int16_t>((data_size + num_needed) / 2));
        if (!sample_16bit)
          goto error_state;
        if (cf_ReadBytes(reinterpret_cast<uint8_t *>(sample_16bit.get()), data_size, cfptr) != data_size)
          goto error_state;
        for (count = 0; count < data_size / 2; count++) {
          sample_16bit[count] = INTEL_SHORT(sample_16bit[count]);
        }

        if (num_needed)
          memset(sample_16bit.get() + (cksize / 2), 0, num_needed);
      }

      // We found data, clear format flag
      f_fmt = 0; // Multiple waveforms, per file, are allowed
      break;
    }

    // Undefined chunk
    default:
      // mprintf(0, "\nSOUND LOADER: Unknown Chunk in %s, attempting to continue.\n", filename);
      break;
    }
  }

  // Close the file
  file.reset();

  if (sample_8bit != nullptr) {
    LOG_WARNING.printf("Sound file %s is 8bit, please make a 16bit version", filename);
  }

  // Make sure we got data and format information
  if (sample_8bit == nullptr && sample_16bit == nullptr) {
    LOG_WARNING.printf("SOUND LOADER: Did not find data and/or format in %s.", filename);
    goto error_state;
  } else if (sample_8bit != nullptr && !f_high_quality) {
    // Doing the volume clipping with the low quality (8 bit) sound requires some tricky math -- see chris
    for (count = 0; count < (int)sample_length; count++) {
      sample_8bit[count] = ((((int)sample_8bit[count]) - 127) * CLIP_ATTENUATION * percent_volume) + 127;
    }
  } else if (sample_16bit != nullptr && f_high_quality) {
    // Do the volume clipping with the high quality sound
    for (count = 0; count < (int)sample_length; count++) {
      sample_16bit[count] *= CLIP_ATTENUATION * percent_volume;
    }
  } else if (sample_8bit == nullptr && !f_high_quality) {

    sample_8bit.reset(mem_rmalloc<uint8_t>(sample_length));
    if (!sample_8bit)
      goto error_state;

    // Do the volume clipping with the high quality sound
    for (count = 0; count < (int)sample_length; count++) {
      sample_16bit[count] *= CLIP_ATTENUATION * percent_volume;
    }

    // NOTE:  Interesting note on sound conversion:  16 bit sounds are signed (0 biase).  8 bit sounds are unsigned
    // (+128 biase).
    for (count = 0; count < (int)sample_length; count++) {
      sample_8bit[count] = (uint8_t)((((int)sample_16bit[count]) + 32767) >> 8);
    }

    sample_16bit.reset();

  } else if (sample_16bit == nullptr && f_high_quality) {
    if (sample_length > std::numeric_limits<int>::max() / static_cast<int>(sizeof(int16_t)))
      goto error_state;
    sample_16bit.reset(mem_rmalloc<int16_t>(sample_length));
    if (!sample_16bit)
      goto error_state;

    // NOTE:  Interesting note on sound conversion:  16 bit sounds are signed (0 biase).  8 bit sounds are unsigned
    // (+128 biase).
    for (count = 0; count < (int)sample_length; count++) {
      sample_16bit[count] = (((int16_t)sample_8bit[count]) - 128) * 256;
    }

    for (count = 0; count < (int)sample_length; count++) {
      sample_16bit[count] *= CLIP_ATTENUATION * percent_volume;
    }

    sample_8bit.reset();
  }

  // Publish only after the entire file and requested conversion succeeded.
  if (SoundFiles[sound_file_index].sample_8bit)
    mem_free(SoundFiles[sound_file_index].sample_8bit);
  if (SoundFiles[sound_file_index].sample_16bit)
    mem_free(SoundFiles[sound_file_index].sample_16bit);
  SoundFiles[sound_file_index].sample_8bit = sample_8bit.release();
  SoundFiles[sound_file_index].sample_16bit = sample_16bit.release();
  SoundFiles[sound_file_index].sample_length = sample_length;
  SoundFiles[sound_file_index].np_sample_length = np_sample_length;

  // reset use count
  SoundFiles[sound_file_index].use_count = 0;
  SoundFiles[sound_file_index].used = true;

  return 1;

error_state:

  // File and unpublished samples are released by their scope owners.
  return 0;
}

void SoundLoadFree(int sound_file_index) {
  int i = sound_file_index;

  if (SoundFiles[i].used != 0) {
    if (SoundFiles[i].sample_8bit)
      mem_free(SoundFiles[i].sample_8bit);

    if (SoundFiles[i].sample_16bit)
      mem_free(SoundFiles[i].sample_16bit);
  }

  SoundFiles[i].sample_8bit = nullptr;
  SoundFiles[i].sample_16bit = nullptr;
}
