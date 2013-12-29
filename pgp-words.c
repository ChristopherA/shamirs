/*
 * Shamir's secret sharing PGP Words Dumping
 *
 * Copyright (C) 2013 Matt Corallo <git@bluematt.me>
 *
 * This file is part of ASSS (Audit-friendly Shamir's Secret Sharing)
 *
 * ASSS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation,	either version 3 of
 * the License,	or (at your option) any later version.
 *
 * ASSS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.

 * You should have received a copy of the GNU Affero General Public
 * License along with ASSS.  If not,	see
 * <http://www.gnu.org/licenses/>.
 */

#define _POSIX_C_SOURCE 200809L
#include <unistd.h>
#include <stdio.h>
#include <getopt.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

static const char* words[256][2] = {
	// even word,	odd word
	{"aardvark",	"adroitness"},
	{"absurd",		"adviser"},
	{"accrue",		"aftermath"},
	{"acme",		"aggregate"},
	{"adrift",		"alkali"},
	{"adult",		"almighty"},
	{"afflict",		"amulet"},
	{"ahead",		"amusement"},
	{"aimless",		"antenna"},
	{"Algol",		"applicant"},
	{"allow",		"Apollo"},
	{"alone",		"armistice"},
	{"ammo",		"article"},
	{"ancient",		"asteroid"},
	{"apple",		"Atlantic"},
	{"artist",		"atmosphere"},
	{"assume",		"autopsy"},
	{"Athens",		"Babylon"},
	{"atlas",		"backwater"},
	{"Aztec",		"barbecue"},
	{"baboon",		"belowground"},
	{"backfield",	"bifocals"},
	{"backward",	"bodyguard"},
	{"banjo",		"bookseller"},
	{"beaming",		"borderline"},
	{"bedlamp",		"bottomless"},
	{"beehive",		"Bradbury"},
	{"beeswax",		"bravado"},
	{"befriend",	"Brazilian"},
	{"Belfast",		"breakaway"},
	{"berserk",		"Burlington"},
	{"billiard",	"businessman"},
	{"bison",		"butterfat"},
	{"blackjack",	"Camelot"},
	{"blockade",	"candidate"},
	{"blowtorch",	"cannonball"},
	{"bluebird",	"Capricorn"},
	{"bombast",		"caravan"},
	{"bookshelf",	"caretaker"},
	{"brackish",	"celebrate"},
	{"breadline",	"cellulose"},
	{"breakup",		"certify"},
	{"brickyard",	"chambermaid"},
	{"briefcase",	"Cherokee"},
	{"Burbank",		"Chicago"},
	{"button",		"clergyman"},
	{"buzzard",		"coherence"},
	{"cement",		"combustion"},
	{"chairlift",	"commando"},
	{"chatter",		"company"},
	{"checkup",		"component"},
	{"chisel",		"concurrent"},
	{"choking",		"confidence"},
	{"chopper",		"conformist"},
	{"Christmas",	"congregate"},
	{"clamshell",	"consensus"},
	{"classic",		"consulting"},
	{"classroom",	"corporate"},
	{"cleanup",		"corrosion"},
	{"clockwork",	"councilman"},
	{"cobra",		"crossover"},
	{"commence",	"crucifix"},
	{"concert",		"cumbersome"},
	{"cowbell",		"customer"},
	{"crackdown",	"Dakota"},
	{"cranky",		"decadence"},
	{"crowfoot",	"December"},
	{"crucial",		"decimal"},
	{"crumpled",	"designing"},
	{"crusade",		"detector"},
	{"cubic",		"detergent"},
	{"dashboard",	"determine"},
	{"deadbolt",	"dictator"},
	{"deckhand",	"dinosaur"},
	{"dogsled",		"direction"},
	{"dragnet",		"disable"},
	{"drainage",	"disbelief"},
	{"dreadful",	"disruptive"},
	{"drifter",		"distortion"},
	{"dropper",		"document"},
	{"drumbeat",	"embezzle"},
	{"drunken",		"enchanting"},
	{"Dupont",		"enrollment"},
	{"dwelling",	"enterprise"},
	{"eating",		"equation"},
	{"edict",		"equipment"},
	{"egghead",		"escapade"},
	{"eightball",	"Eskimo"},
	{"endorse",		"everyday"},
	{"endow",		"examine"},
	{"enlist",		"existence"},
	{"erase",		"exodus"},
	{"escape",		"fascinate"},
	{"exceed",		"filament"},
	{"eyeglass",	"finicky"},
	{"eyetooth",	"forever"},
	{"facial",		"fortitude"},
	{"fallout",		"frequency"},
	{"flagpole",	"gadgetry"},
	{"flatfoot",	"Galveston"},
	{"flytrap",		"getaway"},
	{"fracture",	"glossary"},
	{"framework",	"gossamer"},
	{"freedom",		"graduate"},
	{"frighten",	"gravity"},
	{"gazelle",		"guitarist"},
	{"Geiger",		"hamburger"},
	{"glitter",		"Hamilton"},
	{"glucose",		"handiwork"},
	{"goggles",		"hazardous"},
	{"goldfish",	"headwaters"},
	{"gremlin",		"hemisphere"},
	{"guidance",	"hesitate"},
	{"hamlet",		"hideaway"},
	{"highchair",	"holiness"},
	{"hockey",		"hurricane"},
	{"indoors",		"hydraulic"},
	{"indulge",		"impartial"},
	{"inverse",		"impetus"},
	{"involve",		"inception"},
	{"island",		"indigo"},
	{"jawbone",		"inertia"},
	{"keyboard",	"infancy"},
	{"kickoff",		"inferno"},
	{"kiwi",		"informant"},
	{"klaxon",		"insincere"},
	{"locale",		"insurgent"},
	{"lockup",		"integrate"},
	{"merit",		"intention"},
	{"minnow",		"inventive"},
	{"miser",		"Istanbul"},
	{"Mohawk",		"Jamaica"},
	{"mural",		"Jupiter"},
	{"music",		"leprosy"},
	{"necklace",	"letterhead"},
	{"Neptune",		"liberty"},
	{"newborn",		"maritime"},
	{"nightbird",	"matchmaker"},
	{"Oakland",		"maverick"},
	{"obtuse",		"Medusa"},
	{"offload",		"megaton"},
	{"optic",		"microscope"},
	{"orca",		"microwave"},
	{"payday",		"midsummer"},
	{"peachy",		"millionaire"},
	{"pheasant",	"miracle"},
	{"physique",	"misnomer"},
	{"playhouse",	"molasses"},
	{"Pluto",		"molecule"},
	{"preclude",	"Montana"},
	{"prefer",		"monument"},
	{"preshrunk",	"mosquito"},
	{"printer",		"narrative"},
	{"prowler",		"nebula"},
	{"pupil",		"newsletter"},
	{"puppy",		"Norwegian"},
	{"python",		"October"},
	{"quadrant",	"Ohio"},
	{"quiver",		"onlooker"},
	{"quota",		"opulent"},
	{"ragtime",		"Orlando"},
	{"ratchet",		"outfielder"},
	{"rebirth",		"Pacific"},
	{"reform",		"pandemic"},
	{"regain",		"Pandora"},
	{"reindeer",	"paperweight"},
	{"rematch",		"paragon"},
	{"repay",		"paragraph"},
	{"retouch",		"paramount"},
	{"revenge",		"passenger"},
	{"reward",		"pedigree"},
	{"rhythm",		"Pegasus"},
	{"ribcage",		"penetrate"},
	{"ringbolt",	"perceptive"},
	{"robust",		"performance"},
	{"rocker",		"pharmacy"},
	{"ruffled",		"phonetic"},
	{"sailboat",	"photograph"},
	{"sawdust",		"pioneer"},
	{"scallion",	"pocketful"},
	{"scenic",		"politeness"},
	{"scorecard",	"positive"},
	{"Scotland",	"potato"},
	{"seabird",		"processor"},
	{"select",		"provincial"},
	{"sentence",	"proximate"},
	{"shadow",		"puberty"},
	{"shamrock",	"publisher"},
	{"showgirl",	"pyramid"},
	{"skullcap",	"quantity"},
	{"skydive",		"racketeer"},
	{"slingshot",	"rebellion"},
	{"slowdown",	"recipe"},
	{"snapline",	"recover"},
	{"snapshot",	"repellent"},
	{"snowcap",		"replica"},
	{"snowslide",	"reproduce"},
	{"solo",		"resistor"},
	{"southward",	"responsive"},
	{"soybean",		"retraction"},
	{"spaniel",		"retrieval"},
	{"spearhead",	"retrospect"},
	{"spellbind",	"revenue"},
	{"spheroid",	"revival"},
	{"spigot",		"revolver"},
	{"spindle",		"sandalwood"},
	{"spyglass",	"sardonic"},
	{"stagehand",	"Saturday"},
	{"stagnate",	"savagery"},
	{"stairway",	"scavenger"},
	{"standard",	"sensation"},
	{"stapler",		"sociable"},
	{"steamship",	"souvenir"},
	{"sterling",	"specialist"},
	{"stockman",	"speculate"},
	{"stopwatch",	"stethoscope"},
	{"stormy",		"stupendous"},
	{"sugar",		"supportive"},
	{"surmount",	"surrender"},
	{"suspense",	"suspicious"},
	{"sweatband",	"sympathy"},
	{"swelter",		"tambourine"},
	{"tactics",		"telephone"},
	{"talon",		"therapist"},
	{"tapeworm",	"tobacco"},
	{"tempest",		"tolerance"},
	{"tiger",		"tomorrow"},
	{"tissue",		"torpedo"},
	{"tonic",		"tradition"},
	{"topmost",		"travesty"},
	{"tracker",		"trombonist"},
	{"transit",		"truncated"},
	{"trauma",		"typewriter"},
	{"treadmill",	"ultimate"},
	{"Trojan",		"undaunted"},
	{"trouble",		"underfoot"},
	{"tumor",		"unicorn"},
	{"tunnel",		"unify"},
	{"tycoon",		"universe"},
	{"uncut",		"unravel"},
	{"unearth",		"upcoming"},
	{"unwind",		"vacancy"},
	{"uproot",		"vagabond"},
	{"upset",		"vertigo"},
	{"upshot",		"Virginia"},
	{"vapor",		"visitor"},
	{"village",		"vocalist"},
	{"virus",		"voyager"},
	{"Vulcan",		"warranty"},
	{"waffle",		"Waterloo"},
	{"wallet",		"whimsical"},
	{"watchword",	"Wichita"},
	{"wayside",		"Wilmington"},
	{"willow",		"Wyoming"},
	{"woodlark",	"yesteryear"},
	{"Zulu",		"Yucatan"}
};

int main(int argc, char* argv[]) {
	char* input_file = NULL;
	int i;
	while ((i = getopt(argc, argv, "f:h?")) != -1) {
		switch(i) {
		case 'h':
		case '?':
			printf("Usage: -f filename outputs filename in PGP words\n");
			printf("No arguments allows PGP words to be input (one per line) and then outputs the file after EOF/^D\n");
			return 0;
			break;
		case 'f':
			input_file = optarg;
			break;
		default:
			printf("Error: Unknown argument: check -?\n");
			return -1;
		}
	}

	if (input_file) {
		FILE* in = fopen(input_file, "rb");
		assert(in);
		unsigned char c;
		while (1) {
			// even word
			if (fread(&c, 1, 1, in) != 1)
				break;
			printf(words[c][0]);
			printf(" ");
			// odd word
			if (fread(&c, 1, 1, in) != 1)
				break;
			printf(words[c][1]);
			printf(" ");
		}
		printf("\n");
	} else {
		char* line = NULL;
		size_t line_size = 0;
		unsigned char* output = (unsigned char*) malloc(1024);
		size_t output_size = 1024, output_index = 0;
		int read;
		int use_odd = 0;
		while ((read = getline(&line, &line_size, stdin)) != -1) {
			// Maybe we should use an actual data structure here.....naaaa
			if (output_index + 1 >= output_size) {
				output_size += 1024;
				output = realloc(output, output_size);
			}
			int word = -1;
			for (int i = 0; i <= 0xff; i++) {
				if (!strncasecmp(words[i][use_odd], line, strlen(words[i][use_odd])))
					word = i;
			}
			if (word == -1)
				fprintf(stderr, "Failed to find %s word %s", use_odd ? "odd" : "even", line);
			else {
				output[output_index++] = (unsigned char)word;
				use_odd = (use_odd + 1) % 2;
			}
		}
		for (size_t i = 0; i < output_index; i++) // lol efficiency, who needs it?
			printf("%c", output[i]);
	}
}
