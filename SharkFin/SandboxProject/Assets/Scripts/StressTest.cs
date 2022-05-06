
using Shark;

namespace Sandbox
{
	public class StressTest
	{
		private TimeStep m_SpawnTimer = 2.0f;
		private TimeStep m_Timer;
		private System.Random m_Rng = new System.Random();

		void OnUpdate(TimeStep ts)
		{
			m_Timer += ts;

			if (m_Timer > m_SpawnTimer)
			{
				Log.Debug("SpawnEntity");
				SpawnEntity();
				m_Timer = 0.0f;
			}

		}

		void SpawnEntity()
		{
			for (uint i = 0; i < 200; i++)
			{
				var testScript = Scene.Instantiate<TestScript>("StressEntity");
				testScript.Offset = new Vector3(
					(float)m_Rng.NextDouble() * 20.0f - 10.0f,
					(float)m_Rng.NextDouble() * 20.0f - 10.0f,
					0.0f
				);
			}
		}

	}

}
