using System;

namespace Shark
{

	public class Asset<T> : IEquatable<T> where T : Asset<T>
	{
		public AssetHandle Handle { get; init; }

		internal Asset()
		{
			Handle = AssetHandle.Invalid;
		}

		internal Asset(AssetHandle handle)
		{
			Handle = handle;
		}

		public override bool Equals(object? obj) => obj is T asset && Equals(asset);

		public bool Equals(T? other)
		{
			if (other is null)
				return false;

			if (ReferenceEquals(this, other))
				return true;

			return Handle == other.Handle;
		}

		public override int GetHashCode() => Handle.GetHashCode();

		public bool IsValid() => Handle.IsValid();

		public static bool IsValid(Asset<T>? asset) => asset is not null && asset.IsValid();
		public static implicit operator bool(Asset<T>? asset) => IsValid(asset);
	}

}
