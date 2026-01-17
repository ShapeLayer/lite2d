#include "glmesh.h"
#include "debug.h"

/**
 * Create the GL mesh from the given ArtMesh.
 * @param m The ArtMesh to create from.
 */
void GLMesh::create(const ArtMesh &m)
{
  vertCount = m.verts.size();
  idxCount = m.indices.size();

  cpuInterleaved.reserve(vertCount * 7);
  for (auto &v : m.verts)
  {
    cpuInterleaved.push_back(v.pos.x);
    cpuInterleaved.push_back(v.pos.y);
    cpuInterleaved.push_back(v.uv.x);
    cpuInterleaved.push_back(v.uv.y);
    cpuInterleaved.push_back(v.color.r);
    cpuInterleaved.push_back(v.color.g);
    cpuInterleaved.push_back(v.color.b);
  }

  glGenVertexArrays(1, &vao);
  glGenBuffers(1, &vbo);
  glGenBuffers(1, &ebo);

  glBindVertexArray(vao);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, cpuInterleaved.size() * sizeof(float),
               cpuInterleaved.data(), GL_DYNAMIC_DRAW);

  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, idxCount * sizeof(uint32_t),
               m.indices.data(), GL_STATIC_DRAW);

  glEnableVertexAttribArray(0); // pos
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(1); // uv
  glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(2 * sizeof(float)));
  glEnableVertexAttribArray(2); // color
  glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void *)(4 * sizeof(float)));

  glBindVertexArray(0);
}

void GLMesh::updatePositions(const std::vector<glm::vec2> &pos)
{
  if (pos.size() != vertCount)
    return;

  for (size_t i = 0; i < vertCount; ++i)
  {
    cpuInterleaved[i * 7 + 0] = pos[i].x;
    cpuInterleaved[i * 7 + 1] = pos[i].y;
  }

  glBindVertexArray(vao);
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("updpos: after VAO bind");
#endif

  glBindBuffer(GL_ARRAY_BUFFER, vbo);
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("updpos: after VBO bind");
#endif

  glBufferSubData(GL_ARRAY_BUFFER, 0,
                  cpuInterleaved.size() * sizeof(float),
                  cpuInterleaved.data());
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("updpos: after BufferSubData");
#endif

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}

void GLMesh::draw() const
{
  glBindVertexArray(vao);
  glDrawElements(GL_TRIANGLES, (GLsizei)idxCount, GL_UNSIGNED_INT, 0);
#if defined(LITE2D_DEBUG) && LITE2D_DEBUG
  checkErr("after draw");
#endif
}
