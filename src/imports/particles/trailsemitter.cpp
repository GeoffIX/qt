#include "trailsemitter.h"
#include "particlesystem.h"
#include "particle.h"
QT_BEGIN_NAMESPACE

TrailsEmitter::TrailsEmitter(QSGItem* parent)
    : ParticleEmitter(parent)
    , m_speed_from_movement(0)
    , m_particle_count(0)
    , m_reset_last(true)
    , m_last_timestamp(0)
    , m_last_particle(0)
{
//    setFlag(ItemHasContents);
}

void TrailsEmitter::setSpeedFromMovement(qreal t)
{
    if (t == m_speed_from_movement)
        return;
    m_speed_from_movement = t;
    emit speedFromMovementChanged();
}

void TrailsEmitter::reset()
{
    m_reset_last = true;
}

void TrailsEmitter::emitWindow(int timeStamp)
{
    if (m_system == 0)
        return;
    if(!m_emitting && !m_burstLeft){
        m_reset_last = true;
        return;
    }

    if (m_reset_last) {
        m_last_emitter = m_last_last_emitter = QPointF(x(), y());
        m_last_timestamp = timeStamp/1000.;
        m_last_particle = ceil(m_last_timestamp * m_particlesPerSecond);
        m_reset_last = false;
    }

    m_particle_count = m_particlesPerSecond * (m_particleDuration / 1000.);

    if(m_burstLeft){
        m_burstLeft -= timeStamp - m_last_timestamp * 1000.;
        if(m_burstLeft < 0){
            timeStamp += m_burstLeft;
            m_burstLeft = 0;
        }
    }
    qreal time = timeStamp / 1000.;


    qreal particleRatio = 1. / m_particlesPerSecond;
    qreal pt = m_last_particle * particleRatio;

    qreal opt = pt; // original particle time
    qreal dt = time - m_last_timestamp; // timestamp delta...
    if(!dt)
        dt = 0.000001;

    // emitter difference since last...
    qreal dex = (x() - m_last_emitter.x());
    qreal dey = (y() - m_last_emitter.y());

    qreal ax = (m_last_last_emitter.x() + m_last_emitter.x()) / 2;
    qreal bx = m_last_emitter.x();
    qreal cx = (x() + m_last_emitter.x()) / 2;
    qreal ay = (m_last_last_emitter.y() + m_last_emitter.y()) / 2;
    qreal by = m_last_emitter.y();
    qreal cy = (y() + m_last_emitter.y()) / 2;

    float sizeAtEnd = m_particleEndSize >= 0 ? m_particleEndSize : m_particleSize;
    qreal emitter_x_offset = m_last_emitter.x() - x();
    qreal emitter_y_offset = m_last_emitter.y() - y();
    while (pt < time) {
        //int pos = m_last_particle % m_particle_count;
        ParticleData* datum = m_system->newDatum(m_system->m_groupIds[m_particle]);
        if(!datum){//skip this emission
            ++m_last_particle;
            pt += particleRatio;
            continue;
        }
        datum->e = this;//###useful?
        ParticleVertex &p = datum->pv;
        qreal t = 1 - (pt - opt) / dt;
        qreal vx =
          - 2 * ax * (1 - t)
          + 2 * bx * (1 - 2 * t)
          + 2 * cx * t;
        qreal vy =
          - 2 * ay * (1 - t)
          + 2 * by * (1 - 2 * t)
          + 2 * cy * t;


        // Particle timestamp
        p.t = pt;
        p.lifeSpan = //Promote to base class?
                (m_particleDuration
                 + ((rand() % ((m_particleDurationVariation*2) + 1)) - m_particleDurationVariation))
                / 1000.0;

        // Particle position
        QRectF boundsRect(emitter_x_offset + dex * (pt - opt) / dt, emitter_y_offset + dey * (pt - opt) / dt
                          , width(), height());
        QPointF newPos = effectiveExtruder()->extrude(boundsRect);
        p.x = newPos.x();
        p.y = newPos.y();

        // Particle speed
        const QPointF &speed = m_speed->sample(newPos);
        p.sx = speed.x()
                + m_speed_from_movement * vx;
        p.sy = speed.y()
                + m_speed_from_movement * vy;

        // Particle acceleration
        const QPointF &accel = m_acceleration->sample(newPos);
        p.ax = accel.x();
        p.ay = accel.y();

        // Particle size
        float sizeVariation = -m_particleSizeVariation
                + rand() / float(RAND_MAX) * m_particleSizeVariation * 2;

        float size = m_particleSize + sizeVariation;
        float endSize = sizeAtEnd + sizeVariation;

        p.size = size;// * float(m_emitting);
        p.endSize = endSize;// * float(m_emitting);

        ++m_last_particle;
        pt = m_last_particle * particleRatio;

        m_system->emitParticle(datum);
    }

    m_last_last_last_emitter = m_last_last_emitter;
    m_last_last_emitter = m_last_emitter;
    m_last_emitter = QPointF(x(), y());
    m_last_timestamp = time;
}


QT_END_NAMESPACE