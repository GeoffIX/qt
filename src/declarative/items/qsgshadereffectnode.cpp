/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <private/qsgshadereffectnode_p.h>

#include <private/qsgshadereffectitem_p.h> // XXX todo
#include <private/qsgtextureprovider_p.h>
#include <private/qsgrenderer_p.h>

QT_BEGIN_NAMESPACE

class QSGCustomMaterialShader : public QSGMaterialShader
{
public:
    QSGCustomMaterialShader(const QSGShaderEffectMaterialKey &key, const QVector<const char *> &attributes);
    virtual void updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect);
    virtual char const *const *attributeNames() const;

protected:
    friend class QSGShaderEffectNode;

    virtual void initialize();
    virtual const char *vertexShader() const;
    virtual const char *fragmentShader() const;

    QSGShaderEffectMaterialKey m_key;
    QVector<const char *> m_attributes;

    QVector<int> m_uniformLocs;
    int m_opacityLoc;
    int m_matrixLoc;
    uint m_textureIndicesSet;
};

QSGCustomMaterialShader::QSGCustomMaterialShader(const QSGShaderEffectMaterialKey &key, const QVector<const char *> &attributes)
    : m_key(key)
    , m_attributes(attributes)
    , m_textureIndicesSet(false)
{
}

void QSGCustomMaterialShader::updateState(const RenderState &state, QSGMaterial *newEffect, QSGMaterial *oldEffect)
{
    Q_ASSERT(newEffect != 0);
    Q_UNUSED(oldEffect);
    Q_UNUSED(newEffect);

    const QSGShaderEffectMaterial *material = static_cast<const QSGShaderEffectMaterial *>(newEffect);

    if (!m_textureIndicesSet) {
        for (int i = 0; i < material->m_textures.size(); ++i)
            m_program.setUniformValue(material->m_textures.at(i).first.constData(), i);
        m_textureIndicesSet = true;
    }

    if (m_uniformLocs.size() != material->m_uniformValues.size()) {
        m_uniformLocs.reserve(material->m_uniformValues.size());
        for (int i = 0; i < material->m_uniformValues.size(); ++i) {
            const QByteArray &name = material->m_uniformValues.at(i).first;
            m_uniformLocs.append(m_program.uniformLocation(name.constData()));
        }
    }

    QGLFunctions *functions = state.context()->functions();
    for (int i = material->m_textures.size() - 1; i >= 0; --i) {
        QPointer<QSGTextureProvider> source = material->m_textures.at(i).second;
        if (!source || !source->texture().texture())
            continue;
        functions->glActiveTexture(GL_TEXTURE0 + i);
        source->bind();
    }

    if (material->m_source.respectsOpacity)
        m_program.setUniformValue(m_opacityLoc, state.opacity());

    for (int i = 0; i < material->m_uniformValues.count(); ++i) {
        const QVariant &v = material->m_uniformValues.at(i).second;

        switch (v.type()) {
        case QVariant::Color:
            m_program.setUniformValue(m_uniformLocs.at(i), qvariant_cast<QColor>(v));
            break;
        case QVariant::Double:
            m_program.setUniformValue(m_uniformLocs.at(i), (float) qvariant_cast<double>(v));
            break;
        case QVariant::Transform:
            m_program.setUniformValue(m_uniformLocs.at(i), qvariant_cast<QTransform>(v));
            break;
        case QVariant::Int:
            m_program.setUniformValue(m_uniformLocs.at(i), v.toInt());
            break;
        case QVariant::Bool:
            m_program.setUniformValue(m_uniformLocs.at(i), GLint(v.toBool()));
            break;
        case QVariant::Size:
        case QVariant::SizeF:
            m_program.setUniformValue(m_uniformLocs.at(i), v.toSizeF());
            break;
        case QVariant::Point:
        case QVariant::PointF:
            m_program.setUniformValue(m_uniformLocs.at(i), v.toPointF());
            break;
        case QVariant::Rect:
        case QVariant::RectF:
            {
                QRectF r = v.toRectF();
                m_program.setUniformValue(m_uniformLocs.at(i), r.x(), r.y(), r.width(), r.height());
            }
            break;
        case QVariant::Vector3D:
            m_program.setUniformValue(m_uniformLocs.at(i), qvariant_cast<QVector3D>(v));
            break;
        default:
            break;
        }
    }

    if ((state.isMatrixDirty()) && material->m_source.respectsMatrix)
        m_program.setUniformValue(m_matrixLoc, state.combinedMatrix());
}

char const *const *QSGCustomMaterialShader::attributeNames() const
{
    return m_attributes.constData();
}

void QSGCustomMaterialShader::initialize()
{
    m_opacityLoc = m_program.uniformLocation("qt_Opacity");
    m_matrixLoc = m_program.uniformLocation("qt_ModelViewProjectionMatrix");
}

const char *QSGCustomMaterialShader::vertexShader() const
{
    return m_key.vertexCode.constData();
}

const char *QSGCustomMaterialShader::fragmentShader() const
{
    return m_key.fragmentCode.constData();
}


bool QSGShaderEffectMaterialKey::operator == (const QSGShaderEffectMaterialKey &other) const
{
    return vertexCode == other.vertexCode && fragmentCode == other.fragmentCode && className == other.className;
}

uint qHash(const QSGShaderEffectMaterialKey &key)
{
    return qHash(qMakePair(qMakePair(key.vertexCode, key.fragmentCode), key.className));
}


QHash<QSGShaderEffectMaterialKey, QSharedPointer<QSGMaterialType> > QSGShaderEffectMaterial::materialMap;

QSGShaderEffectMaterial::QSGShaderEffectMaterial()
{
    setFlag(Blending, true);
}

QSGMaterialType *QSGShaderEffectMaterial::type() const
{
    return m_type.data();
}

QSGMaterialShader *QSGShaderEffectMaterial::createShader() const
{
    return new QSGCustomMaterialShader(m_source, m_source.attributeNames);
}

int QSGShaderEffectMaterial::compare(const QSGMaterial *other) const
{
    return this - static_cast<const QSGShaderEffectMaterial *>(other);
}

void QSGShaderEffectMaterial::setProgramSource(const QSGShaderEffectProgram &source)
{
    m_source = source;
    m_type = materialMap.value(m_source);
    if (m_type.isNull()) {
        m_type = QSharedPointer<QSGMaterialType>(new QSGMaterialType);
        materialMap.insert(m_source, m_type);
    }
}

void QSGShaderEffectMaterial::setUniforms(const QVector<QPair<QByteArray, QVariant> > &uniformValues)
{
    m_uniformValues = uniformValues;
}

void QSGShaderEffectMaterial::setTextures(const QVector<QPair<QByteArray, QPointer<QSGTextureProvider> > > &textures)
{
    m_textures = textures;
}

const QVector<QPair<QByteArray, QPointer<QSGTextureProvider> > > &QSGShaderEffectMaterial::textures() const
{
    return m_textures;
}

void QSGShaderEffectMaterial::updateTextures() const
{
    for (int i = 0; i < m_textures.size(); ++i) {
        QSGTextureProvider *source = m_textures.at(i).second;
        if (source)
            source->updateTexture();
    }
}


QSGShaderEffectNode::QSGShaderEffectNode()
    : m_meshResolution(1, 1)
    , m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    QSGNode::setFlag(UsePreprocess, true);
    setGeometry(&m_geometry);
}

QSGShaderEffectNode::~QSGShaderEffectNode()
{
}

void QSGShaderEffectNode::setRect(const QRectF &rect)
{
    m_rect = rect;
    m_dirty_geometry = true;
}

QRectF QSGShaderEffectNode::rect() const
{
    return m_rect;
}

void QSGShaderEffectNode::setResolution(const QSize &res)
{
    m_dirty_geometry = true;
    m_meshResolution = res;
}

QSize QSGShaderEffectNode::resolution() const
{
    return m_meshResolution;
}

void QSGShaderEffectNode::update()
{
    if (m_dirty_geometry) {
        updateGeometry();
        m_dirty_geometry = false;
    }
}

void QSGShaderEffectNode::markDirtyTexture()
{
    markDirty(DirtyMaterial);
}

void QSGShaderEffectNode::updateGeometry()
{
    int vmesh = m_meshResolution.height();
    int hmesh = m_meshResolution.width();

    QSGGeometry *g = geometry();
    if (vmesh == 1 && hmesh == 1) {
        if (g->vertexCount() != 4)
            g->allocate(4);
        QSGGeometry::updateTexturedRectGeometry(g, m_rect, QRectF(0, 0, 1, 1));
        return;
    }

    g->allocate((vmesh + 1) * (hmesh + 1), vmesh * 2 * (hmesh + 2));

    QSGGeometry::TexturedPoint2D *vdata = g->vertexDataAsTexturedPoint2D();

    QRectF dstRect = m_rect;
    QRectF srcRect(0, 0, 1, 1);
    for (int iy = 0; iy <= vmesh; ++iy) {
        float fy = iy / float(vmesh);
        float y = float(dstRect.top()) + fy * float(dstRect.height());
        float ty = float(srcRect.top()) + fy * float(srcRect.height());
        for (int ix = 0; ix <= hmesh; ++ix) {
            float fx = ix / float(hmesh);
            vdata->x = float(dstRect.left()) + fx * float(dstRect.width());
            vdata->y = y;
            vdata->tx = float(srcRect.left()) + fx * float(srcRect.width());
            vdata->ty = ty;
            ++vdata;
        }
    }

    quint16 *indices = (quint16 *)g->indexDataAsUShort();
    int i = 0;
    for (int iy = 0; iy < vmesh; ++iy) {
        *(indices++) = i + hmesh + 1;
        for (int ix = 0; ix <= hmesh; ++ix, ++i) {
            *(indices++) = i + hmesh + 1;
            *(indices++) = i;
        }
        *(indices++) = i - 1;
    }

    markDirty(QSGNode::DirtyGeometry);
}

void QSGShaderEffectNode::preprocess()
{
    Q_ASSERT(material());
    static_cast<QSGShaderEffectMaterial *>(material())->updateTextures();
}

QT_END_NAMESPACE