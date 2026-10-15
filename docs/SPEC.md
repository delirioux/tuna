# Tuna — firmware for the M-VAVE FM-1

Oct 6, 2026 · delirioux

> Exported from the Claude doc "Tuna — firmware for the M-VAVE FM-1" on 7 Oct 2026. The doc is the working
> copy; this file is its snapshot in the repo.

## Goal

Tuna is a generative firmware for the M-VAVE FM-1: press play and it improvises endlessly within a key, while you steer with knobs and keys.

No existing FM-1 firmware does this. The other forks (Felucca, SLOOP, Groove OS, X0X, FM-1+VA) are grooveboxes or synths you program yourself. Tuna builds on Felucca's engines and hardware layer, with its own generator, sequencer and GUI on top.

## Specification

All choices were settled on 6 October 2026.

| Component | Choice |
| --- | --- |
| Base | Own firmware: own generator, sequencer and GUI on Felucca's engines and hardware layer |
| Character | Selectable per preset, from ambient and drones to rhythmic and melodic |
| Start | Only on play |
| Engines | All Felucca engines |
| Sampler | Felucca's SAMPLE (multisampled instruments, 3 user slots), SLICE (break or own sample, one slice per key) and GRAIN (granular from built-in or user samples); upload from PC and on-device recording with trim; the generator can play samples as a part |
| Parts | 3 synth parts (bass, melody, pad) plus percussion, within 8 voices. Percussion is a switch per composer: off, period (the composer's own orchestra: timpani on tonic and dominant, Mahler's march snare, Dvořák's triangle and tambourine, and so on) or kit (a modern kit driven by Marbles). Switching it never changes a note |
| Resonator | Rings-style, as a send effect on the mix, as far as the CPU allows |
| Chaos | Separate knobs for rhythm, pitch and density, plus a macro knob |
| Memory | Deja vu (phrases recur), switchable on and off |
| Scales | All 12 keys × church modes, plus extras (see Scales); historical tunings and a 12-note user table |
| Harmony | Moves along well-known progressions, or follows one of 89 composer signatures; switchable per preset |
| Keys | 1 key sets the root, more keys set a chord |
| Freeze | Convert a generated phrase into an editable pattern |
| P-locks and trig conditions | In both generator and sequencer |
| Form | Endless, with optional evolution timer |
| MIDI | MIDI out per part |
| USB audio | Stereo mix only |
| Screen | Visualisation (notes as falling particles), parameters while turning |
| Presets | 16 slots, full state including seed |
| Publishing | Personal use first; if published, GPL-3.0 with source applies |
| Name | Tuna |

## Composer signatures

A signature is a set of habits the harmonizer follows: chord vocabulary and movement, accompaniment, melodic shape and ornaments, rhythm and phrasing. It evokes a style; it does not quote works. All of it is tables and rules: one signature is 216 bytes of flash, so 100 composers take about 22 KB. Bach and Mozart come first, because their rules are the most explicit.

89 composers and styles, grouped by era; living composers' manners are styles named after their sound; each era is one build batch. The first column says what makes the composer recognisable, the second how Tuna plays it. Rules since 7 Oct 2026: a melody resolves to the tonic only where that fits the composer; odd meters are beat groups (3+3+2, 5/4, 7/8, changing meters); octatonic and other non-diatonic scales are allowed; historical tunings (meantone, Werckmeister, Pythagorean, just) are allowed; instrumentation may change during a piece. Existing signatures (Bach, Mozart, Beethoven, Brahms, Mahler, Chopin, Satie, Debussy, Dvořák, Saint-Saëns) stay as they are and gain instrumentation.

### 1 — Medieval and Renaissance

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Hildegard von Bingen | Soaring single-line chant, range over two octaves, a leap of a fifth up then stepwise descent, melismas, no meter | Chant melody over a drone (final and fifth), D Dorian or E Phrygian, free rhythm, solo voice and drone string, Pythagorean tuning, always ends on the final |
| Thomas Tallis | Tudor polyphony, modal, false relations (major and minor third side by side), slow suspensions, Amen endings | Modal harmony, counter-melody polyphony, suspensions, false relations, choir, quarter-comma meantone, plagal endings with a Picardy third |
| Giovanni Pierluigi da Palestrina | The model of smooth counterpoint: steps, leaps recovered by step, prepared suspensions, pure consonance | Modal, stepwise profile, suspensions on strong beats, counter-melody, choir, meantone, calm 4/2 tactus |
| Gregorio Allegri | Miserere: chanted verses alternate with a harmonised verse and its high ornamented soprano | Alternating periods: chant on one chord, then a four-part verse with a high melody and turns; G minor, choir, meantone |
| Claudio Monteverdi | Expressive dissonance over basso continuo, stile concitato (rapid repeated notes), triple-time ritornelli | Continuo (bass, organ or harpsichord chords, theorbo), early functional harmony, repeated-note bursts, 3/2 ritornello periods, cornett and strings, meantone |
| Jan Pieterszoon Sweelinck | Organ variations on songs, echo fantasias, the falling chromatic fourth, running figuration | A Aeolian, running sixteenths over modal counterpoint, echo periods (the phrase again, softer), Picardy closes, organ and harpsichord, meantone |

### 2 — Baroque

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Dietrich Buxtehude | Stylus phantasticus: free toccata flourishes over pedal points alternating with fugal sections | Alternating periods: fast runs over a pedal, then counter-melody counterpoint; E minor, organ, Werckmeister |
| Johann Pachelbel | Canon: an eight-chord ground repeated, three voices in canon, figuration growing richer | Static eight-chord cycle, ground bass, canon two bars behind, density rising with each pass, strings and organ |
| Henry Purcell | Ground basses (the falling chromatic lament), sighing appoggiaturas, 3/2, English cross-relations | G minor 3/2, the chromatic lament ground under its chords, sighs, harpsichord and strings |
| Antonio Vivaldi | Motor rhythms, ritornello, sequences round the circle of fifths, repeated-note bass, violin arpeggios | Fast sixteenths, strong falling-fifth sequences, repeated-eighth bass, tremolo or arpeggio strings, solo violin, terraced loud and soft |
| Tomaso Albinoni | Adagio: slow pizzicato walking bass, organ, a sighing violin line | G minor, slow walking pizzicato bass, organ chords, violin with sighs |
| Johann Sebastian Bach | (built) | + harpsichord, organ or strings by period; Werckmeister tuning |
| George Frideric Handel | Ceremonial D major, trumpets and drums, homophonic blocks, dotted rhythms, the grand pause before the Amen | D major, plain I–IV–V, arpeggio fanfare melody, repeated tutti chords, trumpets and strings, timpani, pause then plagal ending |
| Domenico Scarlatti | Keyboard sonatas: hand-crossing leaps, repeated notes, crushed dissonant acciaccaturas, Spanish guitar strums, binary form | Fast 3/8, wide leaps, repeated notes, strummed chords, Phrygian-dominant and minor periods, each half repeated, harpsichord and guitar |
| Jean-Philippe Rameau | French ornaments (pincés, ports de voix), the added-sixth chord, tambourin dances over a drone | Many trills and mordents, IV with added sixth, drone bass in fast 2/4, flute and harpsichord, tabor drum |
| Giovanni Battista Pergolesi | Stabat Mater: two high voices in thirds, chains of suspensions, walking bass, sighs | F minor, melody doubled a third below, many suspensions, walking bass, chromatic sighs, strings and organ |

### 3 — Classical

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Christoph Willibald Gluck | Noble simplicity: a plain, long flute melody over triplet strings | D minor, flute over triplet broken chords, plain I–IV–V, few ornaments, strings and harp |
| Joseph Haydn | Wit and surprise: sudden fortissimo, folk-like tunes, one theme developed, minuets, general pauses | G major, tuneful repeated-note melody, very high motif development, rare huge accents, surprise pauses, string quartet or small orchestra |
| Wolfgang Amadeus Mozart | (built) | + piano, strings, winds by period |
| Ludwig van Beethoven | (built) | + orchestra by period |

### 4 — Early Romantic

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Franz Schubert | Song melody, sudden major–minor shifts, third-related keys, repeated-note and guitar-like accompaniment | B-flat major, singable melody with turns, mode mixture, third-related chords, repeated triplets, piano with clarinet or voice |
| Gioachino Rossini | Rossini crescendo, sparkling staccato tunes, galop rhythm, I–V harmony, banda percussion | Fast 2/4, plain I and V, staccato repeated notes, strong crescendo, woodwinds and strings, bass drum, cymbal, triangle |
| Hector Berlioz | Idée fixe, irregular phrase lengths, bold changing orchestration, bells and timpani | One recurring idea, major and minor in turn, tremolo periods, sudden accents and echoes, orchestration changes every period, rich timpani and bells |
| Felix Mendelssohn | Elfin staccato scherzos, 6/8 tarantella, Songs Without Words over flowing arpeggios | A major 6/8, light staccato, repeated woodwind chords, elegant secondary dominants, strings and flute |
| Fanny Mendelssohn | Lied-like piano pieces, long-breathed melodies, chromatic inner voices, bold modulations | A-flat major, broad 8-bar lines, more chromatic and modulating than Felix, wide broken chords, solo piano, rubato |
| Frédéric Chopin | (built) | + solo piano |
| Robert Schumann | Syncopation across the bar, dotted rhythms, inner voices, Florestan and Eusebius in turn | F major, strong syncopation, dotted pairs, inner counter-voice, periods alternate impetuous and dreamy, piano |
| Clara Schumann | Romanzes: lyrical minor-key line over virtuoso arpeggios, octave doubling, poised forms | G minor, triplet arpeggios over two octaves, chromatic appoggiaturas, octave leaps, piano and violin |
| Franz Liszt | Virtuoso runs, diminished-seventh tremolos, Hungarian scale, melody in the middle wrapped in arpeggios | D-flat major with Hungarian minor periods, wide arpeggios, diminished sevenths and augmented colour, turns, strong rubato, piano |

### 5 — Romantic

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Giuseppe Verdi | Oom-pah-pah accompaniment, big unison tunes, martial brass, emphatic V–I cadences | E-flat 3/4 oom-pah-pah, singable arching melody, voice, brass and strings, the banda's bass drum and cymbal |
| Richard Wagner | Endless melody, leitmotifs, the half-diminished Tristan chord, deceptive cadences, huge swells | Rarely resolves, chromatic vocabulary, deceptive endings, 16-bar periods, leitmotif sequences, tremolo strings and brass, timpani rolls |
| Charles Gounod | Ave Maria over Bach's broken-chord prelude, sweet diatonic melody, organ | F major, Bach-prelude figure, long plain melody, organ and violin, plagal Amen |
| Johann Strauss II | Viennese waltz: oom-pah-pah with the second beat early, sixths in the tune, bright major | D major 3/4, bass on 1, chords on 2 and 3, Viennese lilt, leaping melody, strings and horns, light triangle and snare |
| Anton Bruckner | Tremolo openings from nothing, brass chorales, long crescendos, general pauses, the 2+3 Bruckner rhythm | D minor, tremolo strings on a pedal, brass chorale periods between, general pauses, octave themes, plagal emphasis, timpani rolls |
| Johannes Brahms | (built) | + strings, horn, clarinet by period |
| Georges Bizet | Habanera rhythm, chromatic descending melody, Spanish colour, vivid orchestra | D minor to D major, 2/4 habanera bass, chromatic descents, castanets and tambourine, oboe, flute, strings |
| César Franck | Chromatic organ harmony, constant modulation by sliding voices, canon, cyclic themes | F-sharp minor, chromatic vocabulary, frequent modulation, canon at the octave, organ and violin, resolves about half the time |
| Max Bruch | Rhapsodic singing violin or cello lines over warm sustained orchestra | G minor, wide lyrical solo line, sustained strings and horn, mode mixture, strong rubato |
| Camille Saint-Saëns | (built) | + piano, violin, organ by period |
| Modest Mussorgsky | Promenade in alternating 5/4 and 6/4, modal folk tunes, parallel chords, bells, raw harmony | Meter cycle 5/4 + 6/4, pentatonic tune, modal, parallel chords, trumpet and brass; bell periods with a tolling bass between |
| Pyotr Ilyich Tchaikovsky | Sequences rising to climax, falling scales, waltzes, celesta and harp, pedal points | B minor or D major, sequenced motif, downward melodic bias, harp arpeggios, celesta colour, big swells, cymbal climaxes |
| Antonín Dvořák | (built) | + strings, cor anglais, horn by period |
| Bedřich Smetana | Vltava: flowing two-flute water figure in 6/8, a broad E minor folk tune | E minor 6/8, flowing sixteenth figure in flutes, folk-like melody with raised seventh, strings, light triangle |
| Edvard Grieg | Hardanger drone fifths, Lydian fourth, falling chromatic inner line, pastoral pentatonic Morning | E major 6/8, pentatonic melody, drone, flute and oboe in turn, Lydian and minor periods |
| Jules Massenet | Méditation: violin over harp arpeggios, sensuous chromatic appoggiaturas | D major, slow violin with chromatic appoggiaturas and expressive leaps, harp arpeggios, added sixths and ninths, rubato |
| Nikolai Rimsky-Korsakov | Orchestral colour, solo violin arabesques over harp, octatonic and augmented-second colours | E minor with octatonic periods, arabesques with augmented seconds, violin and harp, orchestration changes, triangle and tambourine |

### 6 — Late Romantic and Impressionism

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Charles-Marie Widor | Toccata: unbroken staccato sixteenth broken chords above, the theme in long pedal notes below | F major, toccata figure in the upper part, melody low in long notes, full organ, big final cadence |
| Gabriel Fauré | Pavane: pizzicato pulse, flute line, modal flat seventh, sidestepping modulations, restraint | F-sharp minor, pizzicato bass on the beat, off-beat chords, flute, modal minor dominant, sidesteps, soft dynamics |
| Edward Elgar | Nobilmente: broad melodies with rising sixths and sevenths, sequences, rich strings | E-flat major, wide expressive leaps, suspensions, strings and brass, big swells, timpani and cymbal |
| Giacomo Puccini | Soaring melody doubled in octaves, pentatonic and whole-tone colour, added-sixth endings | A-flat major, melody doubled an octave below, octave leaps, pentatonic periods, add6 and add9 chords, strong rubato, harp |
| Gustav Mahler | (built) | + orchestra by period |
| Richard Strauss | Leaping horn-call themes, sudden side-slips to distant keys, rich polyphony, timpani fanfares | C or E major, leaps up the arpeggio, chromatic mediants, key side-slips, horns and strings, timpani, resolves about two thirds of the time |
| Jean Sibelius | Long pedal points, string ostinatos, swinging horn fifths, slow motivic growth, brass chorales | E-flat major, very slow harmony, pedal bass, oscillating string figure, horn motif in fourths and fifths, built-in slow growth |
| Sergei Rachmaninoff | Lush wide chords, bell-like tolling, narrow long-breathed melancholic melody, chromatic inner descent | C minor, wide 4-note voicings, tolling low octaves, narrow stepwise melody in long notes, 7th and 9th chords, piano with strings |
| Gustav Holst | Mars: 5/4 col legno ostinato, dissonant brass, building menace | 5/4 (3+2), ostinato on G with the Mars rhythm (col legno clicks, a low drum), quartal and octatonic chords, brass, a slow theme building; rarely resolves |
| Ralph Vaughan Williams | Modal folk melody, parallel triads gliding in strings, Lark Ascending arabesques | G Dorian or Mixolydian, planing triads, pentatonic folk melody, solo violin arabesques, modal endings |
| Claude Debussy | (built) | + flute, harp, strings by period |
| Erik Satie | (built) | + solo piano |
| Maurice Ravel | Boléro snare ostinato and changing solo instruments over one crescendo, jazz-tinged 7th to 11th chords | C major 3/4, Boléro snare pattern, C–G ostinato bass, one melody passed from instrument to instrument, crescendo, maj7, 9th and 13th chords, blue notes |

### 7 — Modern

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Béla Bartók | Bulgarian rhythms (3+3+2, 2+2+3), acoustic and octatonic scales, percussive repeated chords, fourth chords | 3+3+2, acoustic or octatonic scale, quartal chords, percussive chords on group starts, narrow folk melody, piano and strings; seldom resolves |
| Igor Stravinsky | Changing meters, polychords, octatonic, irregular accents, narrow repeated folk cells, ostinatos | Changing meter cycle, octatonic, polychords (two triads a tritone apart), short ostinato bass, irregular accents, bassoon and brass, bass drum |
| Sergei Prokofiev | Wrong-note melodies that slip to a distant key and back, motoric rhythms, dotted marches, wide leaps | E minor, dotted march, leaps over an octave, sudden chord side-slips, staccato repeated chords, brass and strings, snare |
| Francis Poulenc | Music-hall cheek against sacred solemnity, diatonic tunes over 7ths and 9ths, abrupt endings | C major, jazz 7th and 9th vocabulary, stride-like chords, periods alternate cheeky (piano, clarinet) and solemn (choir, organ), short endings |
| George Gershwin | Blue notes, syncopation, Charleston 3+3+2, stride piano, 9th and 13th chords, ii–V–I and tritone substitutes | B-flat major, blues-inflected melody, swing, stride or walking bass, jazz vocabulary, piano, clarinet, brass, brushed kit, ends on a 6/9 chord |
| Carl Orff | O Fortuna: driving ostinato, repeated chords, narrow chant-like tune, massive choir and drums, sudden pp to ff | D minor, static ostinato harmony, alternating D–A bass, repeated choir chords, narrow melody, terraced dynamics, timpani, bass drum, crash |
| Joaquín Rodrigo | Aranjuez: strummed guitar, Andalusian cadence, ornamented cor anglais melody | B minor, Phrygian-dominant colour, Andalusian i–VII–VI–V, strummed guitar chords, turns, cor anglais melody |
| Aaron Copland | Open fifths and wide spacing, pandiatonic chords, fanfare leaps of fourths and fifths, hoe-down syncopation | C major, sus and add9 chords, wide open voicings, leaps of fourths and fifths, some syncopation, brass, strings, clarinet, timpani |
| Aram Khachaturian | Sabre Dance: furious repeated notes, xylophone, chromatic oscillation, Armenian augmented seconds | Fast 2/4, repeated-note ostinato, narrow chromatic melody, harmonic-minor colour, xylophone, brass, snare and cymbals |
| Erich Wolfgang Korngold | Viennese lushness in Hollywood: heroic brass fanfares, chromatic added-note harmony, lyrical violin | D major, romantic chromatic vocabulary with add9, heroic leaps, tremolo strings, brass, cymbal and timpani |
| Maurice Duruflé | Gregorian chant harmonised with modal ninth chords, flexible meter, organ | D Dorian, chant melody, m9, maj7 and add6 chords, changing meters, organ and choir, plagal endings |
| Dmitri Shostakovich | Sardonic waltz and march, snare ostinato build-ups, dark modes with lowered degrees | C minor, Phrygian colour, waltz 3/4 oom-pah-pah with saxophone and chromatic slides, sharp accents, snare and cymbal |
| Olivier Messiaen | Modes of limited transposition, birdsong, added-value rhythms, ecstatic slow chords, added sixths | F-sharp major with mode 2 colour, added-sixth and ninth chords, added-value meter (2+3+2+3+3), very slow praise and birdsong periods in turn, organ, bells, ondes-like lead |
| Benjamin Britten | Sea Interludes: high thin line far above low brass chords, harp, bitonal touches, Lydian | A major, high melody over a low brass chorale, harp periods between, Lydian periods, flute and strings, timpani and cymbal |
| Samuel Barber | Adagio for Strings: changing meters, stepwise line rising to one climax, suspensions, ends on the dominant | B-flat minor, 3/2 and 4/2 in turn, stepwise long notes, suspensions, one big swell, strings only, often ends away from the tonic |
| Astor Piazzolla | Nuevo tango: 3+3+2, bandoneon, chromatic falling bass, percussive hits, melancholic minor | A minor, 3+3+2 accents, chromatic descending bass, percussive chords on group starts, bandoneon and violin, m6 and dim7 chords |

### 8 — Minimalism and holy minimalism

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Simeon ten Holt | Canto Ostinato: cells repeated many times in 3+3+2+2, slow harmonic shifts, consonant, several pianos | 3+3+2+2 eighths, ostinato cell with small changes, chord cycle switching every few periods, pianos and marimba, no cadences |
| Henryk Górecki | Very slow, two chords swaying for minutes, canon building from the bass, a plain modal song | A Aeolian, 40 bpm, two-chord cycle, canon an octave below, narrow modal melody, strings and voice, one long swell |
| John Tavener | Ison drone, Byzantine chant with augmented seconds, silences, a final blaze of major | Drone, chant on the Byzantine scale, choir, just intonation, a major chord bloom at period ends |

### 9 — Film and musical

| Composer | Fingerprint | Tuna |
| --- | --- | --- |
| Ennio Morricone | Whistle, twangy guitar, wordless soprano, slow builds on simple minor cycles, whip cracks | A minor, i–VI–III–VII cycle, long leaping melody, voice, guitar, choir, side-stick cracks and bells, slow build |

### 10 — Contemporary

Styles named after their sound rather than a composer, no year: minimal first, then film and stage.

| Style | Fingerprint | Tuna |
| --- | --- | --- |
| TINTINNABULI | Tintinnabuli: a stepwise voice and a voice on the tonic triad, bell tolls, silence | A Aeolian or F major, very slow, tintinnabuli pair, bell on the tonic every few bars, strings or piano and violin, breaths |
| PHASING | Phasing: two identical patterns drifting apart, pulsing chords, marimbas | 12-note pattern, second voice shifts one step every few loops, marimba and piano, static harmony, optional claps |
| ARPEGGIO | Additive process (1, 1-2, 1-2-3), arpeggio ostinatos, three against two, short chord cycles | A minor, additive arpeggio process, i–VI–iv–V cycle, long tones above, organ and flute, no cadences |
| ANTHEM | Warm tuneful choral writing, lilting 6/8, sus4 and add9 colour, harp and organ | F major 6/8, singable melody, sus4 resolving, maj7 and add9, harp arpeggios, organ, flute counter-line |
| BALTIC | Slow string laments, cluster chords opening to consonance, birdsong episodes, folk pentatonic | G minor, add9 and sus2 chords, pentatonic lament in long notes, pedal bass, occasional high birdsong, strings and solo violin |
| SHIMMER | Choral clusters (added seconds), slow homophony, gentle mediant shifts | D-flat major, close cluster voicings, add9 and 6/9, stepwise sustained melody, choir throughout, ends on a cluster |
| FANFARE | Heroic fanfares (fifth up, triplet pickups), Lydian wonder, chromatic mediants, marches | B-flat major march, fanfare leaps with triplets, Lydian periods, flat-VI and flat-III, brass and strings, snare, cymbal, glockenspiel |
| SAGA | Pentatonic Celtic tune with tin whistle; modal brass and choir | D major and Dorian, pentatonic lilting melody, flat-VII chords, whistle, strings, choir, horn |
| SHOWTUNE | Pop-rock ballads, a key change up at the last chorus, chromatic organ riff, 7/4 sections, add9 and sus4 | D minor, rock kit, pop progressions with add9 and sus, the key going up a semitone each cycle (twice, then home) |
| MELANCHOLY | Simple melancholic minor melodies, sparse piano, flute, organ and choir, slow waltz | B minor 3/4, plain stepwise melody repeated, sparse downbeat chords, piano, flute, organ, choir |

Status 6 Oct 2026: foundation built (planned functional harmony with half and authentic cadences and secondary dominants, meter, walking and root bass, Alberti figures, counter-melody without parallel fifths or octaves, appoggiatura, suspension, trill). Status 7 Oct 2026: all ten signatures built, plus a motif engine (each period states a short idea and develops it as sequences, inversions and varied endings; the answer restates it). Every signature passes the checks over 40 seeds: in key, half cadences on the dominant, arrivals on the tonic by step, bass on chord roots, no parallel fifths or octaves.

Status 7 Oct 2026 (afternoon): all 89 signatures built and checked. New in the engine for them: beat-group meters and meter cycles, swing and the waltz lilt; six chord vocabularies (classic, romantic, jazz, modal, modern, pop and choral), fixed chord cycles and drones; open endings (the tonic only where it fits); colour periods in any scale, the parallel mode, side-slips, modulations; chant, tintinnabuli, looped patterns with phasing, birdsong; ostinato, lament-line, drone, tolling and other basses; oom-pah, tremolo, organum, canon, figures, the additive process, polychords, strummed and open voicings; contrast periods, echoes, general pauses; drum patterns; historical tunings; instruments changing per period. Every composer passes 10 seeds of 4 minutes with all checks (in key, arrivals on the tonic or on a tone of the open chord, half cadences, bass roots, no parallel fifths in counterpoint).

Memory: a signature is now 424 bytes (89 take 38 KB of flash; packing the fields could halve that). The generator's state is 27 KB of RAM on the host, less on the 32-bit chip. CPU (instruction counts against Felucca's heaviest reference mix): 0.58x on average over all 89, at worst 1.15x for half a second (Duruflé: choir and organ); seven choir-heavy composers pass 1.0x at their peak. The fallback if the chip runs short: an organ or pad in place of the choir in the third part.

## Scales and progressions

Every scale works on all 12 roots. Scala (microtonal tunings) is out of scope; a 12-note Scala file (one tuning per pitch class) can be loaded as the user tuning.

| Group | Scales |
| --- | --- |
| Church modes | Ionian (major), Dorian, Phrygian, Lydian, Mixolydian, Aeolian (natural minor), Locrian |
| Minor variants | Harmonic minor, melodic minor |
| Pentatonic | Major pentatonic, minor pentatonic |
| Other | Blues |
| Symmetric | Whole tone, octatonic (half-whole and whole-half: Messiaen's mode 2), Messiaen's mode 3 |
| Folk and colour | Acoustic (Lydian dominant), Hungarian minor, double harmonic (Byzantine), Phrygian dominant |
| Tunings | Equal, quarter-comma meantone, Werckmeister III, Pythagorean, 5-limit just, and a user table; laid on the piece's tonic |

Harmony follows switchable progressions. A first set:

| Progression | Character |
| --- | --- |
| I–V–vi–IV | Pop, upbeat |
| ii–V–I | Jazz, resolving |
| i–VI–III–VII | Epic minor |
| i–VII–VI–V (Andalusian cadence) | Spanish, dramatic |
| I–IV–I–V | Folk, simple |
| Modal vamp (one or two chords) | Ambient, static; suits Dorian, Lydian, Mixolydian |
| I–bVII–IV (Mixolydian) | Rock, open |

The chord change rate is adjustable. The list can grow without changing the generator.

## Architecture

Tuna builds everything the user experiences itself, and reuses only the hardware layer and sound sources of [Felucca](https://github.com/hugelton/Felucca) (GPL-3.0). Rewriting those layers would take weeks and add no extra sound.

| Layer | Origin | Contents |
| --- | --- | --- |
| GUI | New | Particle visualisation, parameter overlay, menus |
| Generator | New | Port of Mutable Marbles' T and X generators (MIT), wrapped with chaos knobs, deja vu and seed |
| Music theory | New | Scales, chords, progressions |
| Sequencer | New | Freeze, p-locks, trig conditions, evolution timer |
| Presets | New | 16 slots including seed |
| Engines and effects | Felucca | All engines, including PHYS (modal and string, fixed-point), FM6, VA, granular; reverb and other FX |
| Hardware layer | Felucca (`firmware/hal/`) | Screen, keys and knobs, flash, USB audio, MIDI UART |
| Installation | Felucca | Package format and web installer; back to factory firmware via the M-VAVE updater |

The hardware is a JieLi AC79 chip (pi32v2 architecture): dual core, 240 MHz by default (320 MHz is the family maximum), with a single-precision FPU. The stock firmware already renders its voices on the second core. Budget: about 5,440 cycles per output sample per core. Felucca's DSP is fixed-point (Q15); ported Mutable code can stay floating-point.

Mutable Instruments DSP (Plaits, Rings, Clouds, Braids, Peaks) and the hardware figures above come from [Lunar Modulator](https://github.com/ip2k/lunar-modulator) (MIT), which has ported that code to the FM-1 but has not yet run on real hardware.

## Build plan

Phases 1 to 6 can be done without hardware using `hostsim`; only phase 8 waits for the FM-1.

- [x] **Phase 1 — Music theory core.** Scales and progressions, verified with unit tests. Done 6 Oct 2026: 108 tests pass.
- [x] **Phase 2 — Generator.** Port of Mutable Marbles (T and X/Y generators, deja vu, quantizer) to C, plus a generator for 3 parts. Done 6 Oct 2026: bit-exact against the original over 20,000 random scenarios; notes weighted to the current chord, always in key; a seed replays exactly. Harmonizer added the same day: Marbles makes the random choices, musical rules turn them into notes (stepwise melody with phrases and cadences, bass on roots and approach notes, voice-led pad chords, beat accents); raw Marbles mode stays available. Melodies form question-and-answer periods that resolve on the tonic, over the tonic chord, approached by step. The chaos knobs and macro get their mapping in phase 6.
- [x] **Phase 3 — Engine coupling.** Done 7 Oct 2026: Tuna plays Felucca's real engines (a sound per composer), drums and timpani on track 4 (timpani as retuned toms), humanized timing and velocity, Tuna at a quarter of the audio rate. Instruction counts against Felucca's heaviest reference mix: every composer 0.34–0.71× on average, 0.50–0.90× at its worst half second; garden's grain pad 1.07×. Keys for root and chord move to phase 6 with the controls.
- [x] **Phase 4 — Sequencer.** Done 7 Oct 2026: freeze records the period you hear into an editable pattern (up to 384 notes) and loops it; freeze and unfreeze land on period boundaries. Steps in both pattern and generator take trig conditions (probability, ratio, fill, first, previous, and their negations) and up to 3 p-locks (velocity, pitch steps, octave, length, engine params E0–E7, level, pan, sends); sound locks are restored at the part's next note. The evolution timer shapes an energy arc and modulates by pivot chord (major to the dominant, minor to the relative major) and back home. 148 sequencer checks pass; default output is byte-identical to phase 3.
- [x] **Phase 5 — Presets.** Done 7 Oct 2026: all 89 composers and styles browsable in 10 eras (Renaissance to Contemporary), ordered by year of birth. 8 character presets without a composer, from STILL to DRIVE. 16 slots save the whole state (composer, every setting, seed, evolution, steps with conditions and p-locks, a frozen pattern, a name, sounds per track) in a versioned, CRC-checked record of ~300 bytes, at most 3.6 KB with a full pattern; loading plays exactly the music that was saved. The slots live in Felucca's 4 project objects (4 slots each, A/B sectors): a power cut at any byte keeps the old copy. 1127 preset checks pass; default output byte-identical.
- [x] **Phase 6 — GUI.** Done 7 Oct 2026, Tuna only (Felucca's screens replaced): Tuna's own pages in Felucca's look (its fonts, icons, 10 palettes, FLAT and LINE). HOME: the notes falling as particles (colour per part, size per velocity), the chord and key, and four chaos knobs RHYTHM, PITCH, DENSITY, MACRO (0 = the composer byte for byte). PRESETS browses the library piece by piece, ALGORITHM era by era; while playing a new piece comes in at the end of the period. Keys: a tap sets the root from the next chord, several held set that chord. OCT−/OCT+ step the seed, REC locks the phrase (deja vu). Pages: library, key/scale/tuning/memory, time, parts, evolution, freeze, mix, reverb, save/load on the 16 white keys, menu. Tested on the host with Felucca's `gfx.c`: 132 UI checks plus a layout check over every page, palette and composer; PNGs and GIFs of each page. Wiring the hardware (knobs, keys, LEDs, the audio loop) follows in phase 8.
- [x] **Phase 7 — MIDI and resonator.** Done 7 Oct 2026. MIDI out (ENV page): bass, melody, pad, timpani and drums on channels 1, 2, 3, 4 and 10; instrument families as General MIDI programs; sound locks as controllers; historical tunings as pitch bend on the one-note parts; clock with start and stop; every note closed on stop; the FM-1's own sound can be switched off for MIDI only. Resonator (LFO page): six sympathetic strings on the mix in the manner of Rings, fixed point at half rate, tuned to the chord, the key or the root and gliding as the harmony moves, saved with each slot; about 0.12× the reference mix when on (Debussy's worst half second 1.02×), nothing when off. 102 MIDI and 18 resonator checks; the USB send itself is wired in phase 8.
- [ ] **Phase 8 — On the FM-1.** Install toolchain, flash, measure CPU on the chip. Then fine-tune each composer by analysing their music or MIDI files of their work (chord transitions, intervals, rhythm). Prepared 7 Oct 2026, before delivery: the firmware glue (knobs, keys, LEDs, flash, USB MIDI out, the resonator after the mix) behind FELUCCA_TUNA=1; Tuna's own single-precision maths, since the firmware has no C library; no doubles anywhere (checked by compiling for a single-precision-only CPU: nothing left but memcpy and memset); and Tuna in Felucca's browser emulator, the same code on a simulated FM-1, with a Node test (boot, play, browse, root, pages, stop, save).

## Development environment

`hostsim` renders the same DSP code as the firmware to WAV, without hardware or the JieLi toolchain. Tested on 5 October 2026: building takes seconds, 4 s of audio renders in about 20 ms.

```
git clone --depth 1 https://github.com/hugelton/Felucca.git
cd Felucca && mkdir -p build/gen build/host
python3 tools/gen_tables.py build/gen/felucca_tables.h
python3 tools/gen_fm6_patches.py build/gen/felucca_fm6.h
python3 tools/gen_samples.py build/gen/felucca_samples.h
cc -O2 -w -Ibuild/gen -Ifirmware/src -o build/host/hostsim tests/hostsim.c -lm
BENCH=1 SECS=4 ./build/host/hostsim 9 0 1 phys.wav   # engine 9 = PHYS
```

CPU cost per preset is listed in `tests/cpu_baseline.txt` (instructions per sample, measured on a Mac). That is a relative measure; the real measurement follows on the AC79.

Phase 8 needs: the JieLi toolchain (`tools/get_toolchain.sh`, from pkgman.jieliapp.com), the JieLi AC79 SDK (from gitee.com), and Python with Pillow and fontTools. On Windows via WSL2. Claude's sandbox cannot reach either domain, so that step happens on your own PC.

## Risks and open items

The biggest risk is CPU: X0X hits 102% in the worst case with simpler engines, and Felucca shares 8 voices across all parts.

| Risk | Consequence | Fallback |
| --- | --- | --- |
| Resonator as send effect too heavy | Audio dropouts | Engine per part only, or drop it |
| Generator plus 3 parts too heavy | Less polyphony | Mono pad part, or 2 synth parts |
| Felucca's use of the second core unknown (stock firmware renders voices there) | Headroom uncertain | Investigate in phase 8 |
| Felucca changes fast | Merge work on updates | Pin to one version, update selectively |
| Exact chip type and clock unconfirmed | Estimates may be off | Read the board on delivery |
| Choir-heavy composers (Duruflé, SHIMMER, Allegri, Tavener) peak at 1.06–1.15x the reference mix | Possible dropouts at their loudest moments | Organ or pad in place of the choir in the third part; measure on the chip in phase 8 |
| JieLi compiler emits doubles or libm calls the host build never showed | Link errors or slow soft-float on the chip | Check the first FM-1 build's undefined symbols; add the missing function to tuna_libm.c |

- [ ] Read the exact chip type once the FM-1 arrives
- [ ] Extend the progression list to taste
- [ ] Publishing: decide later
