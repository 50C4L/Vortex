#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <glm/glm.hpp>

namespace graphics
{
	class AbstractCamera
	{
	public:
		virtual ~AbstractCamera() = default;

		virtual void SetPosition( const glm::vec3& position ) = 0;

		virtual const glm::vec3& GetPosition() const = 0;

		virtual const glm::mat4& GetViewMatrix() const = 0;
		virtual const glm::mat4& GetProjectionMatrix() const = 0;
		virtual glm::mat4 GetViewProjectionMatrix() const
		{
			return GetProjectionMatrix() * GetViewMatrix();
		}
	};

	class OrthographicCamera : public AbstractCamera
	{
	public:
		OrthographicCamera( float left, float right, float bottom, float top, float near, float far );

		void SetPosition( const glm::vec3& position ) override;

		const glm::vec3& GetPosition() const override;

		const glm::mat4& GetViewMatrix() const override;
		const glm::mat4& GetProjectionMatrix() const override;
	
	private:
		glm::vec3 mPosition;
		glm::mat4 mViewMatrix;
		glm::mat4 mProjectionMatrix;
	};
}

#endif // _CAMERA_H_