

#include "driver/i2s_std.h"     

#ifndef DEBUG                   // user can define favorite behaviour ('true' displays addition info)
#  define DEBUG true            // <- define your preference here  
#  define DebugPrint(x);        if(DEBUG){Serial.print(x);}   /* do not touch */
#  define DebugPrintln(x);      if(DEBUG){Serial.println(x);} /* do not touch */ 
#endif


#define I2S_WS      22         
#define I2S_SD      35          
#define I2S_SCK     33     




#define SAMPLE_RATE             16000  
#define BITS_PER_SAMPLE         8      
#define GAIN_BOOSTER_I2S        45 


i2s_std_config_t  std_cfg = 
{ .clk_cfg  =   // instead of macro 'I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),'
  { .sample_rate_hz = SAMPLE_RATE,
    .clk_src = I2S_CLK_SRC_DEFAULT,
    .mclk_multiple = I2S_MCLK_MULTIPLE_256,
  },
  .slot_cfg =   // instead of macro I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
  { // hint: always using _16BIT because I2S uses 16 bit slots anyhow (even in case I2S_DATA_BIT_WIDTH_8BIT used !)
    .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,   // not I2S_DATA_BIT_WIDTH_8BIT or (i2s_data_bit_width_t) BITS_PER_SAMPLE  
    .slot_bit_width = I2S_SLOT_BIT_WIDTH_AUTO, 
    .slot_mode = I2S_SLOT_MODE_MONO,              // or I2S_SLOT_MODE_STEREO
    .slot_mask = I2S_STD_SLOT_RIGHT,              // use 'I2S_STD_SLOT_LEFT' in case L/R pin is connected to GND !
    .ws_width =  I2S_DATA_BIT_WIDTH_16BIT,           
    .ws_pol = false, 
    .bit_shift = true,   // using [.bit_shift = true] similar PHILIPS or PCM format (NOT 'false' as in MSB macro) ! ..
    .msb_right = false,  // .. or [.msb_right = true] to avoid overdriven and distorted signals on I2S microphones
  },
  .gpio_cfg =   
  { .mclk = I2S_GPIO_UNUSED,
    .bclk = (gpio_num_t) I2S_SCK,
    .ws   = (gpio_num_t) I2S_WS,
    .dout = I2S_GPIO_UNUSED,
    .din  = (gpio_num_t) I2S_SD,
    .invert_flags = 
    { .mclk_inv = false,
      .bclk_inv = false,
      .ws_inv = false,
    },
  },
};

// [re_handle]: global handle to the RX channel with channel configuration [std_cfg]
i2s_chan_handle_t  rx_handle;


// [myWAV_Header]: selfmade WAV Header:
struct WAV_HEADER 
{ char  riff[4] = {'R','I','F','F'};                        /* "RIFF"                                   */
  long  flength = 0;                                        /* file length in bytes                     ==> Calc at end ! */
  char  wave[4] = {'W','A','V','E'};                        /* "WAVE"                                   */
  char  fmt[4]  = {'f','m','t',' '};                        /* "fmt "                                   */
  long  chunk_size = 16;                                    /* size of FMT chunk in bytes (usually 16)  */
  short format_tag = 1;                                     /* 1=PCM, 257=Mu-Law, 258=A-Law, 259=ADPCM  */
  short num_chans = 1;                                      /* 1=mono, 2=stereo                         */
  long  srate = SAMPLE_RATE;                                /* samples per second, e.g. 44100           */
  long  bytes_per_sec = SAMPLE_RATE * (BITS_PER_SAMPLE/8);  /* srate * bytes_per_samp, e.g. 88200       */ 
  short bytes_per_samp = (BITS_PER_SAMPLE/8);               /* 2=16-bit mono, 4=16-bit stereo (byte 34) */
  short bits_per_samp = BITS_PER_SAMPLE;                    /* Number of bits per sample, e.g. 16       */
  char  dat[4] = {'d','a','t','a'};                         /* "data"                                   */
  long  dlength = 0;                                        /* data length in bytes (filelength - 44)   ==> Calc at end ! */
} myWAV_Header;


bool flg_is_recording = false;         // only internally used

bool flg_I2S_initialized = false;      // to avoid any runtime errors in case user forgot to initialize



// ------------------------------------------------------------------------------------------------------------------------------

bool I2S_Record_Init() 
{  
 
  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);
  
  i2s_new_channel(&chan_cfg, NULL, &rx_handle);     
  i2s_channel_init_std_mode(rx_handle, &std_cfg);   
  i2s_channel_enable(rx_handle);                    


  
  flg_I2S_initialized = true;                       

  return flg_I2S_initialized;  
}




bool Record_Start( String audio_filename ) 
{
  if (!flg_I2S_initialized)    
  {  Serial.println( "ERROR in Record_Start() - I2S not initialized, call 'I2S_Record_Init()' missed" );    
     return false;
  }
  
  if (!flg_is_recording)
  { 
    flg_is_recording = true;
    
    if (SD.exists(audio_filename)) 
    {  SD.remove(audio_filename); DebugPrintln("\n> Existing AUDIO file removed.");
    }  else {DebugPrintln("\n> No AUDIO file found");}
    

    File audio_file = SD.open(audio_filename, FILE_WRITE);
    audio_file.write((uint8_t *) &myWAV_Header, 44);
    audio_file.close(); 
    
    DebugPrintln("> WAV Header generated, Audio Recording started ... ");
    return true;
  }
  
  if (flg_is_recording)  
  { 
   
    int16_t audio_buffer[1024];    
    uint8_t audio_buffer_8bit[1024];
   
    size_t bytes_read = 0;
    i2s_channel_read(rx_handle, audio_buffer, sizeof(audio_buffer), &bytes_read, portMAX_DELAY);

    
    if ( GAIN_BOOSTER_I2S > 1 && GAIN_BOOSTER_I2S <= 64 );  
    for (int16_t i = 0; i < ( bytes_read / 2 ); ++i)        
    {   audio_buffer[i] = audio_buffer[i] * GAIN_BOOSTER_I2S;  
    }

    if (BITS_PER_SAMPLE == 8) // in case we store a 8bit WAV file we fill the 2nd array with converted values
    { for (int16_t i = 0; i < ( bytes_read / 2 ); ++i)        
      { audio_buffer_8bit[i] = (uint8_t) ((( audio_buffer[i] + 32768 ) >>8 ) & 0xFF); 
      }
    }
    


    File audio_file = SD.open(audio_filename, FILE_APPEND);
    if (audio_file)
    {  
       if (BITS_PER_SAMPLE == 16) 
       {  audio_file.write((uint8_t*)audio_buffer, bytes_read);
       }        
       if (BITS_PER_SAMPLE == 8)  
       {  audio_file.write((uint8_t*)audio_buffer_8bit, bytes_read/2);
       }  
       audio_file.close(); 
       return true; 
    }  
    
    if (!audio_file) 
    { Serial.println("ERROR in Record_Start() - Failed to open audio file!"); 
      return false;
    }    
  }  
}




bool Record_Available( String audio_filename, float* audiolength_sec ) 
{
 
  if (!flg_is_recording) 
  {   return false;   
  }
  
  if (!flg_I2S_initialized)   
  {  return false;
  }
  
 
  if (flg_is_recording) 
  { 

    
    File audio_file = SD.open(audio_filename, "r+");
    long filesize = audio_file.size();
    audio_file.seek(0); myWAV_Header.flength = filesize;  myWAV_Header.dlength = (filesize-8);
    audio_file.write((uint8_t *) &myWAV_Header, 44);
    audio_file.close(); 
    
    flg_is_recording = false;  // important: this is done only here
    
    *audiolength_sec = (float) (filesize-44) / (SAMPLE_RATE * BITS_PER_SAMPLE/8);   // return Audio length (via reference) 
     
    DebugPrintln("> ... Done. Audio Recording finished.");
    DebugPrint("> New AUDIO file: '" + audio_filename + "', filesize [bytes]: " + (String) filesize);
    DebugPrintln(", length [sec]: " + (String) *audiolength_sec);
    
    return true;   
  }  
}
