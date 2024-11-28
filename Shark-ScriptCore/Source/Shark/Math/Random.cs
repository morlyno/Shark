
namespace Shark
{
	public static class Random
	{
		private static System.Random m_Random = new System.Random();

		public static float Float() => (float)m_Random.NextDouble();
		public static float Float(float min, float max) => Float() * (max - min) + min;

		public static int Int() => m_Random.Next();
		public static int Int(int min, int max) => m_Random.Next(min, max);
	}
}
