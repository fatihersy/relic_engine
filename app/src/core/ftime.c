#include "ftime.h"
#include "time.h"

#include "core/fmemory.h"

const u32 random_table[RANDOM_TABLE_NUMBER_COUNT] = {
  5276, 2157, 6243, 5730, 9791, 3544, 4787, 6206, 3437, 2804, 6252, 5637, 3609, 3516, 2191, 6658, 
  9321, 7627, 3750, 8294, 9911, 1734, 2113,  909, 8978, 8762, 6331, 3796, 3017, 1918, 5878,  238, 
  1710, 8994, 4774,  358, 8986, 2550, 2751, 6535,  264, 4439,  169, 5870, 1879, 3833, 8935, 7172, 
  3410, 7844, 3479, 5076, 8254, 6268, 1420,  647, 5864, 5785, 5053, 8560, 5917,   43, 8633, 8009, 
  7896, 6444, 5704, 5216, 3279, 6996, 2157, 7662, 8855, 5314, 1541, 8399, 8229, 8409, 8414, 5566, 
  1057, 1032, 7781, 3422, 1620, 9307, 7272, 2494, 8749, 5919,  559, 7440, 8729, 4784, 2070, 3284, 
  1101, 2786, 6298, 2508, 4529, 6497, 2755,  874, 2829, 1939, 9288, 3899, 1982, 2152, 5100, 6630, 
  1987, 2299, 2403, 5259, 3466, 7170, 5345, 4253, 8870, 6739, 7863, 3504, 2544, 5499,  716, 2499, 
  9743, 4110, 4743, 6243, 3694, 1207, 6322, 1079, 5982, 5106, 1694, 7934, 4952, 4897, 4504,  763, 
   381, 8773, 6004, 8715, 3717, 7630, 7452, 9668,  983, 5545, 6944, 8159, 7128, 8465, 6068, 8726, 
  3816, 4043, 5887, 6971, 8213, 3054, 4132, 4458, 1349, 9181, 4631, 4425, 6650, 2694, 4081, 3877, 
  1530, 7544,  245, 5636, 8312, 8230, 7186, 1304, 4433,  430, 3093, 8427, 6126, 8290, 7169, 7592, 
  3033, 4524, 3350, 7002, 8991,  476, 3509, 3655, 2699, 9693, 1139, 7654, 8023,  331, 4199, 5974, 
  2583, 1111, 5546,  283, 8167, 9380,  917, 5535, 5446, 3418, 7042, 9092, 3576, 2904, 6962, 1881, 
  6155, 6166, 4940, 2309, 5941, 1877, 9261, 1591, 8257, 3474, 4925, 8116, 5311, 8503, 7756,  695, 
  4562, 4734,  607, 5449, 7456, 4375, 6563, 9116, 9969, 1029, 3515, 8540,  769,  551, 3944,  555, 
  2532, 1427, 9592, 9897, 5031, 1198, 2908, 6819, 4489, 2538, 5043, 8700, 5813, 1724, 5138, 1399, 
  4848, 3892, 4653, 3806,  442, 6316, 9797, 2863, 7323, 5050, 9171, 2687, 6322, 8555, 2508,  786, 
  7736, 5590, 7858, 3951, 9372, 8869, 1589,  628,  989,  208, 6838, 4534, 8145, 6497, 2675, 1059, 
  4241, 6872, 5648, 4601, 2389, 9170, 2486, 9186, 9406, 1634, 9900, 7065,  671,   91,  710, 3764, 
  2204, 7023, 4797, 2451, 6477, 2509,  678,  698, 1396, 4631,  644, 9001, 6695, 7430, 4610, 5765, 
  8054, 2802, 1254, 4596, 7656, 5836, 6646, 3878, 9347, 4034, 7294, 3440, 2718, 8943, 8242, 1571, 
  9239,  834, 2577, 9326, 9267, 4263, 8086, 2631, 2415, 6886, 9964,  310, 1924, 3040, 5746, 9591, 
  8287, 8776,  785, 7466, 7029, 1526,  411, 7910, 7225, 6939,  891, 5392, 1166, 4565, 1105,   17, 
  4697, 7528, 9744, 8548, 3746, 4915, 6946, 2396, 7275, 6752, 8779, 6521, 1172, 8472, 5999, 1660, 
  7561, 3526, 3639, 1851, 4353, 6410, 6030, 2172, 1771, 9858,  622, 9226, 6406, 6122, 1550, 3477, 
  1528,  212, 7021, 2909, 8768,  773, 1427, 5715, 9781, 5196, 2377, 9832, 9661, 1408, 8847, 3695, 
  2090, 3164, 9457, 7632, 3710,  274,  227,  877, 6191, 5921, 7447, 8769, 5592, 5038, 1718, 2121, 
  6744, 7469, 3726, 1653, 1657, 8628, 6289, 4134,  328, 8468, 7811, 7212, 4533, 8943, 8942,  429, 
  9144, 2164, 7863, 6224, 6030, 9377, 2269, 4024, 7386, 6046, 1741, 3326, 7123, 9588, 6829, 6803, 
  8587, 2559, 6640, 7803, 4362, 2045, 5320, 3981, 1792, 9880, 3351, 3528, 7466, 3348, 6868,  888, 
  9733, 5947, 1051, 2797, 9508,  275, 7154,  620, 1166, 2695, 7976,
};

typedef struct time_system_state {
  time_t time;
  struct tm * parsed_time;
  u16 rand_start_index;
  u16 rand_ind;
} time_system_state;

static time_system_state *state;

#define STATE_ASSERT(FUNCTION_NAME_STR, RETURN) if (!state) {                                     \
  TraceLog(LOG_ERROR, "scene_in_game::" FUNCTION_NAME_STR "::In game state was not initialized"); \
  RETURN;                                                                                         \
}

void time_system_initialize(void) {
  if (state) {
    return;
  }
  state = (time_system_state *)allocate_memory_linear(sizeof(time_system_state), true);
  time(&state->time);
  state->parsed_time = localtime(&state->time);
  state->rand_start_index = ((
    (state->parsed_time->tm_hour * state->parsed_time->tm_min) + 
    (state->parsed_time->tm_sec * state->parsed_time->tm_mday)
    ) % RANDOM_TABLE_NUMBER_COUNT
  );
  
}
void update_time(void) {
  STATE_ASSERT("update_time", return;)

  time(&state->time);
  state->parsed_time = localtime(&state->time);
}

i32 get_random(i32 min, i32 max) {
  STATE_ASSERT("get_random", return I32_MAX)

  if (state->rand_ind >= RANDOM_TABLE_NUMBER_COUNT-1) {
    state->rand_ind = 0;
  }
  else state->rand_ind++;

  const u16 ind = (state->rand_start_index + state->rand_ind) % RANDOM_TABLE_NUMBER_COUNT;
  i32 rnd = random_table[ind];
  rnd = (rnd % (max - min + 1)) + min;
  
  return rnd;
}

#undef STATE_ASSERT
