
using System;
using System.Runtime.InteropServices;

namespace Shark
{
    [StructLayout(LayoutKind.Sequential)]
    public struct SoundID
    {
        internal ulong ID;
    }

    public class SoundConfig : Asset<SoundConfig>
    {
    }

    public static class Audio
    {
        public static unsafe SoundID StartPlayback(SoundConfig soundConfig, ulong attachedEntityID)   => InternalCalls.Audio_StartPlayback(soundConfig.Handle, attachedEntityID, false);
        public static unsafe SoundID StartPlayback(SoundConfig soundConfig, Entity attachedEntity)    => InternalCalls.Audio_StartPlayback(soundConfig.Handle, attachedEntity.ID, false);
        public static unsafe SoundID StartPlayback(SoundConfig soundConfig, AudioComponent component) => InternalCalls.Audio_StartPlayback(soundConfig.Handle, component.Entity.ID, true);

        public static unsafe void StopPlayback(SoundID soundID)  => InternalCalls.Audio_StopPlayback(soundID);
        public static unsafe void PausePlayback(SoundID soundID) => InternalCalls.Audio_PausePlayback(soundID);
        public static unsafe void ResuePlayback(SoundID soundID) => InternalCalls.Audio_ResumePlayback(soundID);
        public static unsafe bool IsPlaying(SoundID soundID)     => InternalCalls.Audio_IsPlaying(soundID);
        public static unsafe bool Finished(SoundID soundID)      => InternalCalls.Audio_Finished(soundID);
    }

}
