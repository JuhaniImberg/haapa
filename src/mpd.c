#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mpd/client.h>

#include "mpd.h"
#include "result.h"
#include "config.h"

struct mpd_connection *conn;
struct mpd_status *status;
struct mpd_audio_format *format;
struct mpd_song *song = NULL;
mpd_response *response;

/* call from tick() */
int _mpd_update() {
    int val;
    unsigned int const *val_arr;
    struct mpd_audio_format const *format;
    if(!response)
        response = calloc(sizeof(mpd_response), 1);
    /* TODO: reconnect to MPD if no connection */
    if(response->err) {
        return -1;
    }
    if(!conn)
        conn = mpd_connection_new(mpd_hostname, mpd_port, mpd_timeout);
    if(mpd_pass) {
        mpd_send_password(conn, mpd_pass);
    }
    if(mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        fprintf(stderr, "MPD connection: %s\n", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        response->err = -1;
        return -1;
    }
    mpd_command_list_begin(conn, 1);
    mpd_send_status(conn);
    mpd_send_current_song(conn);
    mpd_command_list_end(conn);

    status = mpd_recv_status(conn);
    if(status == NULL) {
        fprintf(stderr, "MPD update failed!: %s\n", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        response->err = -1;
        return -1;
    }
    if(!response) {
        fprintf(stderr, "MPD update failed!: %s\n", mpd_connection_get_error_message(conn));
        mpd_connection_free(conn);
        response->err = -1;
        return -1;
    }
    for(val = 0; val < 15; val++)
        response->max_arr[val] = 1;

    val = mpd_status_get_state(status);
    response->playing = val==MPD_STATE_PLAY?0:val==MPD_STATE_PAUSE?1:-1;
    val_arr = mpd_connection_get_server_version(conn);
    response->vera = val_arr[0];
    response->verb = val_arr[1];
    response->verc = val_arr[2];
    response->repeat = mpd_status_get_repeat(status);
    response->qv = mpd_status_get_queue_version(status);
    response->ql = mpd_status_get_queue_length(status);
    response->spos = mpd_status_get_song_pos(status);
    response->sels = mpd_status_get_elapsed_time(status);
    response->selms = mpd_status_get_elapsed_ms(status);
    response->slen = mpd_status_get_total_time(status);
    response->sbrate = mpd_status_get_kbit_rate(status);

    format = mpd_status_get_audio_format(status);
    response->afsr = format->sample_rate;
    response->afbits = format->bits;
    response->afchan = format->channels;

    mpd_response_next(conn);
    if(song)
        mpd_song_free(song);
    song = mpd_recv_song(conn);
    if(!song) {
        fprintf(stderr, "Failed to get song data: %s\n",
                        mpd_connection_get_error_message(conn));
        response->err = -1;
    }
    else {
        response->uri = mpd_song_get_uri(song);
        response->artist = mpd_song_get_tag(song, MPD_TAG_ARTIST, 0);
        response->album = mpd_song_get_tag(song, MPD_TAG_ALBUM, 0);
        response->title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        response->track = mpd_song_get_tag(song, MPD_TAG_TRACK, 0);
        response->name = mpd_song_get_tag(song, MPD_TAG_NAME, 0);
        response->date = mpd_song_get_tag(song, MPD_TAG_DATE, 0);
        response->max_arr[7] = response->ql;
        response->max_arr[8] = response->slen;
        response->max_arr[9] = response->slen;
    }
    mpd_response_finish(conn);
    mpd_status_free(status);
    return 0;
}

Result *_mpd_wrap(int i) {
    Result *res = init_res();
    if(!response) {
        res->error = -1;
        return res;
    }
    if(response->err) {
        res->error = response->err;
        return res;
    }
    res->value = response->int_arr[i];
    res->max = response->max_arr[i];
    sprintf(res->string, "%d", response->int_arr[i]);
    return res;
}
Result *_mpd_swrap(int i) {
    Result *res = init_res();
    if(!response) {
        res->error = -1;
        return res;
    }
    if(response->err) {
        res->error = response->err;
        return res;
    }
    strcat(res->string, response->char_arr[i]);
    return res;
}

int mpd_playing() {
    if(!response)
        return 0;
    if(response->err)
        return 0;
    return !response->playing;
}
#define M(a,b) Result *mpd_ ##a() { \
    return _mpd_wrap(b);\
}

M(vera,1)
M(verb,2)
M(verc,3)
M(repeat,4)
M(qv,5)
M(ql,6)
M(spos,7)
M(sels,8)
M(selms,9)
M(slen,10)
M(sbrate,11)
M(afsr,12)
M(afbits,13)
M(afchan,14)
#undef M
#define M(a,b) Result *mpd_ ##a() { \
    return _mpd_swrap(b);\
}
M(uri,0)
M(artist,1)
M(album,2)
M(title,3)
M(track,4)
M(name,5)
M(date,6)
#undef M
