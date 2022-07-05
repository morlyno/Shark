using Shark;

namespace Sandbox
{
	public class EmptyScript
	{
		protected virtual void OnCreate() { Log.Info("EmptyScript.OnCreate"); }
		protected virtual void OnDestroy() { Log.Info("EmptyScript.OnDestroy"); }
		protected virtual void OnUpdate(TimeStep ts) { Log.Info("EmptyScript.OnUpdate"); }
		//protected virtual void OnPhysicsUpdate(TimeStep fixedTimeStep) { Log.Info("EmptyScript.OnPhyisicsUpdate"); }
		protected virtual void OnUIRender() { Log.Info("EmptyScript.OnUIRender"); }
		protected virtual void OnCollishionBegin(Entity entity) { Log.Info("EmptyScript.OnCollishionBegin"); }
		protected virtual void OnCollishionEnd(Entity entity) { Log.Info("EmptyScript.OnCollishionEnd"); }

	}

}
