/* Minimal example of reading values from AD7606 ADC using libiio framework */

#include <stdio.h>
#include <iio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/time.h>

// Streaming devices
static struct iio_device *adc;
static const char *adc_name = "iio:device0";

/* IIO structs required for streaming */
static struct iio_context *ctx;
static struct iio_buffer *rxbuf;
static struct iio_channel **channels;

int main(int argc, char **argv)
{
    // Hardware trigger

    unsigned int channel_count = 0;

    unsigned int i, j, major, minor;
    char git_tag[8];
    iio_library_get_version(&major, &minor, git_tag);
    printf("Library version: %u.%u (git tag: %s)\n", major, minor, git_tag);

    printf("* Acquiring IIO context\n");
    ctx = iio_create_default_context();

    printf("* Acquiring device %s\n", adc_name);
    adc = iio_context_find_device(ctx, adc_name);

    printf("* Initializing IIO streaming channels:\n");
    for (i = 0; i < iio_device_get_channels_count(adc); ++i)
    {
        struct iio_channel *chn = iio_device_get_channel(adc, i);
        if (iio_channel_is_scan_element(chn))
        {
            printf("%s\n", iio_channel_get_id(chn));
            channel_count++;
        }
    }

    channels = calloc(channel_count, sizeof *channels);
    for (i = 0; i < channel_count; ++i)
    {
        struct iio_channel *chn = iio_device_get_channel(adc, i);
        if (iio_channel_is_scan_element(chn))
            channels[i] = chn;
    }

    /*
        This is where the trigger should be intialized but it doesn't work.
        Manually assigning custom trigger like sysfs trigger or hardware timer tigger fails with invalid arugment.
    */
    /*
    printf("* Acquiring trigger %s\n", trigger_str);
    trigger = iio_context_find_device(ctx, trigger_str);
    if (!trigger || !iio_device_is_trigger(trigger)) {
        perror("No trigger found (try setting up the iio-trig-hrtimer module)");
    }
    */

    printf("* Enabling IIO streaming channels for buffered capture\n");
    for (i = 0; i < channel_count; ++i)
    {
        iio_channel_enable(channels[i]);
    }

    /* Measure sample time */
    struct timeval tval_before, tval_after, tval_result;

    while (true)
    {
        gettimeofday(&tval_before, NULL);

        /*
            For some reason, I could not get change the trigger for AD7606, so every iio_buffer_refill hangs on the next try
            Creating and destroying buffer for every read is a workaround.
        */

        rxbuf = iio_device_create_buffer(adc, 1, false);

        ssize_t nbytes_rx = iio_buffer_refill(rxbuf);

        /* Convert the pointer to int16_t ADC raw values */
        int16_t *buff_start = (int16_t *)iio_buffer_first(rxbuf, channels[0]);

        /* Don't read the last channel ID (timestamp) */
        for (i = 0; i < channel_count - 1; ++i)
        {
            printf("%s ", iio_channel_get_id(channels[i]));
            printf(" %d ", *(buff_start + i));
        }

        printf("\n");

        iio_buffer_destroy(rxbuf);

        gettimeofday(&tval_after, NULL);

        sleep(1);

        gettimeofday(&tval_after, NULL);
        timersub(&tval_after, &tval_before, &tval_result);

        printf("Time elapsed: %ld.%06ld\n", (long int)tval_result.tv_sec, (long int)tval_result.tv_usec);
    }

    return 0;
}
