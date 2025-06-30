/*
 * MIT License
 *
 * Copyright (c) 2025 Nikolay Nedelchev
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.
 */

#ifndef QE_APPLEII_H__
#define QE_APPLEII_H__

#include <qe_utils.h>
#include <qe_6502.h>

static const uint16_t qeaii_width = 280;
static const uint16_t qeaii_height = 192;
static const uint16_t qeaii_frame_size = (280 * 192) / 7; // 7 pixels per byte
static const uint16_t qeaii_pixels_per_clock = 7;
static const uint16_t qeaii_total_clocks_per_line = 65;
static const uint16_t qeaii_dummy_lines = 70;
static const uint16_t qeaii_clocks_per_line_visible_pixels = qeaii_width / qeaii_pixels_per_clock;

static const uint16_t qeaii_disk_tracks = 35;
static const uint16_t qeaii_disk_track_size = 0x1a00;
static const uint8_t qeaii_flag_cpu_error = (1 << 7);
static const uint8_t qeaii_flag_new_frame = (1 << 0);

typedef struct
{
    qe_bool is_text;
    qe_bool is_mixed;
    qe_bool is_hires;
    uint8_t bitmap[280 * 192 / 7];
} qeaii_frame_t;

typedef struct
{
    qe_bool readonly;
    qe_bool changed;
    uint8_t data[0x1A00 * 35];
} qeaii_diskette_t;

typedef struct
{
    uint8_t data[0x10000];
} qeaii_memory_t;

struct qeaii_appleII;
typedef qe_bool (*user_handler_fn)( struct qeaii_appleII* pc );

typedef struct
{
    uint8_t key;
    uint8_t key_register;
} qeaii_keyboard_t;

typedef struct
{
    qe_bool is_text;
    qe_bool is_mixed;
    qe_bool is_page2;
    qe_bool is_hires;

    qe_bool blink;

    uint16_t line;
    uint16_t col;
    qe_word32_t offsets;

    uint8_t current_frame;
    uint16_t frame_pos;
    qeaii_frame_t frames[2];
    uint8_t font_lines[8][64]; //[line][code]
    uint8_t ifont_lines[8][64]; //[line][code]
} qeaii_videocard_t;

typedef struct
{
    uint16_t ticks[512];
    uint64_t start_cycle;
    uint64_t end_cycle;
    uint16_t tick_count;
    uint8_t speaker_state;
} qeaii_speaker_frame_t;

typedef struct
{
    qeaii_speaker_frame_t frames[2];
    uint8_t current_frame;
    uint8_t last_value;
} qeaii_speaker_t;

typedef struct
{
    uint16_t firts_rom_address;
    qeaii_memory_t memory;
} qeaii_bus_t;

typedef struct
{
    qe_bool is_mount;
    qe_bool q6;
    qe_bool q7;
    uint8_t phase;
    uint16_t track;           // from 0 to 34
    uint16_t track_pos;
    qe_bool phases[4];
    qeaii_diskette_t diskette;
} qeaii_drive_state_t;

typedef struct
{
    qe_bool spinning;
    uint8_t active_drive;
    qeaii_drive_state_t drives[2];
} qeaii_driveII_t;

typedef struct qeaii_appleII
{
    qe6502_t cpu;
    qe6502_cycle_t cycle;
    qeaii_keyboard_t kbd;
    qeaii_videocard_t video;
    qeaii_speaker_t speaker;
    qeaii_driveII_t driveII;
    qeaii_bus_t bus;
    qe_bool nmi;
    qe_bool is_ok;
    uint64_t cycle_counter;
    uint8_t stop_flags;
    user_handler_fn ex_video_handler;
    user_handler_fn ex_bus_handler;
    user_handler_fn ex_cpu_handler;
} qeaii_t;

typedef struct
{
    uint8_t mem[0x10000];
    uint8_t font_rom[2048];
    qeaii_diskette_t disk0;
    qe_bool mount_disk0;
    uint16_t first_rom_address;
    user_handler_fn ex_video_handler;
    user_handler_fn ex_bus_handler;
    user_handler_fn ex_cpu_handler;
} qeaii_bootstrap_t;

QE_API
qe_bool qeaii_power_on(qeaii_t* pc,
                     qeaii_bootstrap_t* bootstrap);
QE_API
void qeaii_break(qeaii_t* pc);

QE_API
qeaii_frame_t* qeaii_frame(qeaii_t* pc);


QE_API
uint32_t qeaii_run_instructions(qeaii_t* pc,
                 uint16_t max_instructions);
QE_API
uint32_t qeaii_run_instructions_ex(qeaii_t* pc,
                    uint16_t max_instructions);

#if(QE6502_ENABLE_CYCLE_MERGE != 1)
    QE_API // returns cycles left, not cycles processed
    uint32_t qeaii_run(qeaii_t* pc, uint32_t max_cycles);
    QE_API // returns cycles left, not cycles processed
    uint32_t qeaii_run_ex(qeaii_t* pc, uint32_t max_cycles);
#endif

QE_API
void qeaii_press_key(qeaii_t* pc,
                   uint8_t key);
QE_API
qeaii_speaker_frame_t*
qeaii_speaker_frame(qeaii_t* pc);

QE_API
void qeaii_mount_disk0(qeaii_t* pc,
                     qeaii_diskette_t* diskette);
QE_API
void qeaii_unmount_disk0(qeaii_t* pc);
QE_API
qe_bool qeaii_pc_ok(qeaii_t* pc);
QE_API
qe_bool qeaii_disk_active(qeaii_t* pc);
QE_API
qe_bool qeaii_frame_ready(qeaii_t* pc);

#endif // QE_APPLEII_H__
