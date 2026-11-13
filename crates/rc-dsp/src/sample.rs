//! Sound slot and sound identity definitions.

pub const TOTAL_SOUND_COUNT: usize = 32;

/// Standard sound slot enumeration matching RC Engine sound sets.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum SoundId {
    Idle = 0,
    Rev = 1,
    Start = 2,
    Knock = 3,
    Turbo = 4,
    Wastegate = 5,
    Horn = 6,
    JakeBrake = 7,
    Fan = 8,
    Siren = 9,
    Brake = 10,
    ParkingBrake = 11,
    Shifting = 12,
    Reversing = 13,
    Indicator = 14,
    Coupling = 15,
    Supercharger = 16,
    Uncoupling = 17,
    Sound1 = 18,
    TireSqueal = 19,
    HydraulicPump = 20,
    HydraulicFlow = 21,
    TrackRattle = 22,
    BucketRattle = 23,
    Bell = 24,
    Door = 25,
    Scanner = 26,
    Music = 27,
    Whistle = 28,
    Gun = 29,
    OutOfFuel = 30,
    Others = 31,
}

impl SoundId {
    pub const fn index(&self) -> usize {
        *self as usize
    }

    pub fn from_index(index: usize) -> Option<Self> {
        if index < TOTAL_SOUND_COUNT {
            // Safety: SoundId is #[repr(u8)] from 0 to 31
            Some(unsafe { core::mem::transmute(index as u8) })
        } else {
            None
        }
    }
}

/// A zero-copy sound slot holding a reference to 8-bit signed PCM samples.
#[derive(Debug, Clone, Copy, Default)]
pub struct SoundSlot<'a> {
    pub samples: &'a [i8],
    pub sample_rate: u16,
}

impl<'a> SoundSlot<'a> {
    pub const fn empty() -> Self {
        Self {
            samples: &[],
            sample_rate: 22050,
        }
    }

    pub const fn new(samples: &'a [i8], sample_rate: u16) -> Self {
        Self {
            samples,
            sample_rate,
        }
    }

    /// Parse an 8-byte header `.pcm` binary buffer (matches RC Engine "RP" format).
    pub fn parse_pcm(buf: &'a [u8]) -> Result<Self, ()> {
        if buf.len() < 8 || buf[0] != b'R' || buf[1] != b'P' {
            return Err(());
        }
        let sample_rate = u16::from_le_bytes([buf[2], buf[3]]);
        let sample_count = u32::from_le_bytes([buf[4], buf[5], buf[6], buf[7]]) as usize;
        if buf.len() < 8 + sample_count {
            return Err(());
        }
        // Safety: i8 and u8 have identical size and alignment
        let samples = unsafe {
            core::slice::from_raw_parts(buf[8..8 + sample_count].as_ptr() as *const i8, sample_count)
        };
        Ok(Self {
            samples,
            sample_rate,
        })
    }

    #[inline]
    pub fn len(&self) -> usize {
        self.samples.len()
    }

    #[inline]
    pub fn is_empty(&self) -> bool {
        self.samples.is_empty()
    }
}

/// Collection of 32 sound slots forming a vehicle sound bank.
pub struct SoundBank<'a> {
    pub slots: [SoundSlot<'a>; TOTAL_SOUND_COUNT],
}

impl<'a> Default for SoundBank<'a> {
    fn default() -> Self {
        Self::new()
    }
}

impl<'a> SoundBank<'a> {
    pub const fn new() -> Self {
        Self {
            slots: [SoundSlot::empty(); TOTAL_SOUND_COUNT],
        }
    }

    pub fn set_slot(&mut self, id: SoundId, slot: SoundSlot<'a>) {
        self.slots[id.index()] = slot;
    }

    pub fn get_slot(&self, id: SoundId) -> &SoundSlot<'a> {
        &self.slots[id.index()]
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_parse_pcm_binary() {
        let raw = [
            b'R', b'P',         // Magic
            0x22, 0x56,         // 22050 LE
            0x04, 0x00, 0x00, 0x00, // 4 samples LE
            0x00, 0x10, 0xFE, 0x7F, // 4 samples
        ];

        let slot = SoundSlot::parse_pcm(&raw).expect("valid pcm");
        assert_eq!(slot.sample_rate, 22050);
        assert_eq!(slot.len(), 4);
        assert_eq!(slot.samples[0], 0);
        assert_eq!(slot.samples[1], 16);
        assert_eq!(slot.samples[2], -2);
        assert_eq!(slot.samples[3], 127);
    }
}
