struct Vertex
{
	float position[3];
	float color[3];
};

struct VertexAdvanced {
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec4 tangent;
	glm::vec2 uv;
};