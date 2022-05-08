
namespace Shark
{
	public class CameraComponent : Component
	{
		public enum ProjectionType
		{
			Perspective, Orthographic
		};

		public Matrix4 Projection
		{
			get => InternalCalls.CameraComponent_GetProjection(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetProjection(m_EntityUUID, value);
		}

		public ProjectionType Type
		{
			get => InternalCalls.CameraComponent_GetProjectionType(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetProjectionType(m_EntityUUID, value);
		}

		public void SetPerspective(float aspectratio, float fov, float clipnear, float clipfar) => InternalCalls.CameraComponent_SetPerspective(m_EntityUUID, aspectratio, fov, clipnear, clipfar);
		public void SetOrthographic(float aspectratio, float zoom, float clipnear, float clipfar) => InternalCalls.CameraComponent_SetOrthographic(m_EntityUUID, aspectratio, zoom, clipnear, clipfar);

		public float Aspectratio
		{
			get => InternalCalls.CameraComponent_GetAspectratio(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetAspectratio(m_EntityUUID, value);
		}
		public float PerspectiveFOV
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFOV(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetPerspectiveFOV(m_EntityUUID, value);
		}
		public float PerspectiveNear
		{
			get => InternalCalls.CameraComponent_GetPerspectiveNear(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetPerspectiveNear(m_EntityUUID, value);
		}
		public float PerspectiveFar
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFar(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetPerspectiveFar(m_EntityUUID, value);
		}

		public float OrthographicZoom
		{
			get => InternalCalls.CameraComponent_GetOrthographicZoom(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetOrthographicZoom(m_EntityUUID, value);
		}
		public float OrthographicNear
		{
			get => InternalCalls.CameraComponent_GetOrthographicNear(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetOrthographicNear(m_EntityUUID, value);
		}
		public float OrthographicFar
		{
			get => InternalCalls.CameraComponent_GetOrthographicFar(m_EntityUUID);
			set => InternalCalls.CameraComponent_SetOrthographicFar(m_EntityUUID, value);
		}

		public CameraComponent(UUID owner)
			: base(owner)
		{
		}
	}
}
