#include <Arduino.h>
#include <string.h>

const int BUZZER_PIN = 27;

typedef struct {
    const char *note;
    int frequency; // in Hz
} NoteFreq;

typedef struct {
    const char *note;
    int duration; // in milliseconds
} MelodyNote;

typedef struct {
    int bpm; // beast per minute
    int whole;
    int half;
    int quarter;
    int eighth;
    int dotted_whole;
    int dotted_half;
    int dotted_quarter;
    int dotted_eighth;
} NoteDurations;

NoteFreq NOTES[] = {
    {"REST", 0}, {"B0", 31}, {"C1", 33}, {"CS1", 35},
    {"D1", 37}, {"DS1", 39}, {"E1", 41}, {"F1", 44},
    {"FS1", 46}, {"G1", 49}, {"GS1", 52}, {"A1", 55},
    {"AS1", 58}, {"B1", 62}, {"C2", 65}, {"CS2", 69},
    {"D2", 73}, {"DS2", 78}, {"E2", 82}, {"F2", 87},
    {"FS2", 93}, {"G2", 98}, {"GS2", 104}, {"A2", 110},
    {"AS2", 117}, {"B2", 123}, {"C3", 131}, {"CS3", 139},
    {"D3", 147}, {"DS3", 156}, {"E3", 165}, {"F3", 175},
    {"FS3", 185}, {"G3", 196}, {"GS3", 208}, {"A3", 220},
    {"AS3", 233}, {"B3", 247}, {"C4", 262}, {"CS4", 277},
    {"D4", 294}, {"DS4", 311}, {"E4", 330}, {"F4", 349},
    {"FS4", 370}, {"G4", 392}, {"GS4", 415}, {"A4", 440},
    {"AS4", 466}, {"B4", 494}, {"C5", 523}, {"CS5", 554},
    {"D5", 587}, {"DS5", 622}, {"E5", 659}, {"F5", 698},
    {"FS5", 740}, {"G5", 784}, {"GS5", 831}, {"A5", 880},
    {"AS5", 932}, {"B5", 988}, {"C6", 1047}, {"CS6", 1109},
    {"D6", 1175}, {"DS6", 1245}, {"E6", 1319}, {"F6", 1397},
    {"FS6", 1480}, {"G6", 1568}, {"GS6", 1661}, {"A6", 1760},
    {"AS6", 1865}, {"B6", 1976}, {"C7", 2093}, {"CS7", 2217},
    {"D7", 2349}, {"DS7", 2489}, {"E7", 2637}, {"F7", 2794},
    {"FS7", 2960}, {"G7", 3136}, {"GS7", 3322}, {"A7", 3520},
    {"AS7", 3729}, {"B7", 3951}, {"C8", 4186}, {"CS8", 4435},
    {"D8", 4699}, {"DS8", 4978}
};
const int NOTE_COUNT = sizeof(NOTES) / sizeof(NoteFreq);


/*
    description: retrieves and returns the frequency assigned 
                 to the string note
    parameters:
        note (int): pointer to the string note
*/
int getFreq(const char *note){
    for (int i = 0; i < NOTE_COUNT; i++){
        if (strcmp(NOTES[i].note, note) == 0){
            return NOTES[i].frequency;
        }
    }
    return 0; // Default: REST
}

/*
    description: plays the melody on the buzzer by looping
                 through each note and its duration
    parameters:
        melody (in): array of MelodyNote structs
        length (int): number of notes in the array
*/
void playMelody(const MelodyNote *melody, int length) {
    for (int i = 0; i < length; i++) {
        int frequency = getFreq(melody[i].note);
        int duration = melody[i].duration;

        if (frequency == 0) {
            noTone(BUZZER_PIN); // REST
        } else {
            tone(BUZZER_PIN, frequency);
        }
        delay(duration);
        noTone(BUZZER_PIN);
        delay(20);
    }
}

/*
    description: initializes the note durations based on provided
                 beats per minute
    parameters:
        bpm (int): beats per minute
*/
NoteDurations note_duration_init(int bpm) {
    NoteDurations nd;
    nd.bpm = bpm;

    nd.whole = 240000/bpm;
    nd.half = 120000/bpm;
    nd.quarter = 60000/bpm;
    nd.eighth = 30000/bpm;
    nd.dotted_whole = nd.whole*3/2;
    nd.dotted_half = nd.half*3/2;
    nd.dotted_quarter = nd.quarter*3/2;
    nd.dotted_eighth = nd.eighth*3/2;

    return nd;
}

NoteDurations jingle_nd = note_duration_init(120);
MelodyNote JingleBells[] = {
    {"E5", jingle_nd.quarter}, {"E5", jingle_nd.quarter}, {"E5", jingle_nd.half},
    {"REST", jingle_nd.quarter},
    {"E5", jingle_nd.quarter}, {"E5", jingle_nd.quarter}, {"E5", jingle_nd.half},
    {"REST", jingle_nd.quarter},

    {"E5", jingle_nd.quarter}, {"G5", jingle_nd.quarter}, {"C5", jingle_nd.quarter}, 
    {"D5", jingle_nd.quarter}, {"E5", jingle_nd.whole},
    {"REST", jingle_nd.quarter},

    {"F5", jingle_nd.quarter}, {"F5", jingle_nd.quarter}, {"F5", jingle_nd.quarter},
    {"F5", jingle_nd.quarter}, {"F5", jingle_nd.quarter}, 
    
    {"E5", jingle_nd.quarter}, {"E5", jingle_nd.quarter}, {"E5", jingle_nd.eighth}, 
    {"E5", jingle_nd.eighth}, {"D5", jingle_nd.eighth}, {"D5", jingle_nd.quarter}, 
    {"E5", jingle_nd.quarter}, {"D5", jingle_nd.quarter}, {"G5", jingle_nd.half}, 
    {"G5", jingle_nd.whole}, {"REST", jingle_nd.whole},
};

void setup(){
    pinMode(BUZZER_PIN, OUTPUT);
    int jingle_len = sizeof(JingleBells) / sizeof(MelodyNote);
    playMelody(JingleBells, jingle_len);
}

void loop() {

}