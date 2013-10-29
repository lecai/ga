//
// Created by eacan  on 13-9-29 下午1:31.
//
// To change the template use AppCode | Preferences | File Templates.
//


#include "BBMotion.h"
#include "libwebsockets.h"

USING_NS_CC;

/****************************************************************************
* motionInfo 动画属性类
*****************************************************************************/
motionInfo* motionInfo::create()
{
    motionInfo *info= new motionInfo();
    info->autorelease();
    return info;
}

motionInfo* motionInfo::create(const motionInfo& info)
{
    motionInfo *pRet = new motionInfo();
    *pRet = info;
    pRet->autorelease();
    return pRet;
}

/*
根据动画创建自身动作
 */
CCFiniteTimeAction* motionInfo::createAction(CCNode *self, CCNode *parent, const CCPoint& point, CCNode *target, TargetCallBack *callback)
{
    /*判断是移动还是静止*/
    CCFiniteTimeAction *action = NULL;
    CCAnimation *animation = NULL;
    if (frameType == FOLLOW_ANIMATIONS)
    {
        FollowAnimationAction *followAction = FollowAnimationAction::create(target, needTime);
        followAction->isTempTarget = true;
        motionType = OTHER;
        followAction->callBack = callback;
        animation = createAnimation(needTime);

        action = followAction;
    }
    else if(frameType == FOLLOW_FRAMES)
    {
        MotionFollowAction *followAction = MotionFollowAction::create(*this,target);
        action = followAction;
        followAction->callBack = callback;
        followAction->isTempTarget = true;
        motionType = OTHER;
    }
    else if(frameType == TIME_FRAMES && !point.equals(ccp(-1, -1)))
    {
        //移动动画
        MotionMoveAction *moveAction = MotionMoveAction::create(*this, point);
        action=moveAction;
        if (motionType == OTHER)
        {
            moveAction->isTempTarget = true;
        }
    }
    else
    {
        //静止动画
        MotionAction *motionAction = MotionAction::create(animation);
        action = motionAction;
        if (motionType == OTHER)
        {
            motionAction->isTempTarget = true;
        }
    }

    if (!action) return NULL;

    switch (motionType)
    {
        case OTHER:
        {
            CCSprite *temp = CCSprite::create();
            if (temp)
            {
                parent->addChild(temp);
                if (animation)
                {
                    temp->runAction(CCRepeatForever::create(CCAnimate::create(animation)));
                }
            }
            temp->setPosition(self->getPosition());
            action->setTarget(temp);
        }
            break;
    }
    return action;
}

/*
下一个动画序列
 */
motionInfo* motionInfo::addNextMotionInfo(int nextCombineType, const motionInfo& info)
{
    this->nextCombineType = nextCombineType;
    this->nextMotion = motionInfo::create(info);
    return nextMotion;
}

void motionInfo::release()
{
    for (std::vector<CCSpriteFrame*>::iterator iter = frames.begin(); iter != frames.end();++iter)
    {
        if (*iter)
        {
            (*iter)->release();
        }
    }
}

CCAnimation* motionInfo::createAnimation(float time)
{
    CCAnimation *animation = CCAnimation::create();
    for(int i=0;i<frames.size();i++)
    {
        animation->addSpriteFrame(frames[i]);
    }
    if (frames.empty()) return NULL;
    if (time)
    {
        animation->setDelayPerUnit(time/frames.size());
    }
    else
    {
        animation->setDelayPerUnit(needTime/frames.size());
    }
    animation->setRestoreOriginalFrame(true);
    return animation;
}
/****************************************************************************
* MotionMoveAction 移动动作
*****************************************************************************/
void MotionMoveAction::update(float time)
{
    if (m_pTarget)
    {
        m_pTarget->setPosition(ccp(m_startPosition.x + m_delta.x*time,m_startPosition.y + m_delta.y*time));
        CCPoint nowPoint = m_pTarget->getPosition();
        float dw = ccpDistance(nowPoint, m_startPosition);
        float da = ccpDistance(m_endPosition, m_startPosition);//总距离
        int frameSize = this->info.frames.size();
        int nowFrameId = frameSize - 1;
        if (da) nowFrameId = frameSize * (dw/da);
        if (nowFrameId >=0 && nowFrameId < frameSize)
        {
            CCSpriteFrame *frame = this->info.frames[nowFrameId];
            if (frame)
            {
                CCSprite *sprite = (CCSprite*)m_pTarget;
                if (sprite)
                    sprite->setDisplayFrame(frame);
            }
        }
    }
}

MotionMoveAction* MotionMoveAction::create(const motionInfo& info, const CCPoint& position)
{
    MotionMoveAction *pMove = new MotionMoveAction();
    pMove->init(info, position);
    pMove->autorelease();
    return pMove;
}

bool MotionMoveAction::init(const motionInfo& info, const CCPoint& position)
{
    this->info=info;
    initWithDuration(info.needTime,position);
    return true;
}

void MotionMoveAction::stop(void)
{
    CCNode *tempTarget = m_pTarget;
    CCMoveTo::stop();
    info.release();
    if (isTempTarget)
    {
        tempTarget->removeFromParentAndCleanup(true);
    }
}

/****************************************************************************
* MotionFollowAction 跟随动作
*****************************************************************************/
MotionFollowAction* MotionFollowAction::create(const motionInfo& info, CCNode *target)
{
    MotionFollowAction *pMoveTo = new MotionFollowAction();
    pMoveTo->initWithDuration(info.needTime,CCPointZero);
    pMoveTo->autorelease();
    pMoveTo->info=info;
    pMoveTo->initWithAnimation(pMoveTo->info.createAnimation());
    if (target)
    {
        pMoveTo->target = target;
        target->retain();
    }
    return pMoveTo;
}

bool MotionFollowAction::initWithAnimation(CCAnimation *pAnimation)
{
    float singleDuration = pAnimation->getDuration();
    pAnimation->retain();
    if (CCActionInterval::initWithDuration(singleDuration * pAnimation->getLoops()))
    {
        this->m_pAnimation = pAnimation;
        m_pOrigFrame = NULL;
        m_uExecutedLoops = 0;

        m_pSplitTimes.resize(pAnimation->getFrames()->count());

        float accumUnitsOfTime = 0;
        float newUnitOfTimeValue = singleDuration / pAnimation->getTotalDelayUnits();

        CCArray* pFrames = pAnimation->getFrames();
        CCARRAY_VERIFY_TYPE(pFrames, CCAnimationFrame*);

        CCObject* pObj = NULL;
        CCARRAY_FOREACH(pFrames, pObj)
            {
                CCAnimationFrame* frame = (CCAnimationFrame*)pObj;
                float value = (accumUnitsOfTime * newUnitOfTimeValue) / singleDuration;
                accumUnitsOfTime += frame->getDelayUnits();
                m_pSplitTimes.push_back(value);
            }
        return true;

    }
    return false;
}

void MotionFollowAction::startWithTarget(CCNode *pTarget)
{
    CCActionInterval::startWithTarget(pTarget);
    CCSprite *pSprite = (CCSprite*)pTarget;
    m_startPosition = pSprite->getPosition();
    CC_SAFE_RELEASE(m_pOrigFrame);
    if (m_pAnimation->getRestoreOriginalFrame())
    {
        m_pOrigFrame = pSprite->displayFrame();
        m_pOrigFrame->retain();
    }
    m_nNextFrame = 0;
    m_uExecutedLoops = 0;
}

void MotionFollowAction::update(float t)
{
    if (target && target->retainCount()>1)
    {
        m_endPosition = target->getPosition();
        m_delta = ccpSub(m_endPosition, m_startPosition);
    }
    if (m_pTarget)
    {
        m_pTarget->setPosition(ccp(m_startPosition.x + m_delta.x * t,
        m_startPosition.y + m_delta.y * t));

        /*根据时间计算帧*/
        updateAnimation(t);
    }
}

void MotionFollowAction::updateAnimation(float t)
{
    //if it = 1 表示60秒 忽略 animation should finish t = 1
    if (t<1.0f)
    {
        t *= m_pAnimation->getLoops();

        unsigned int loops = (unsigned int)t;
        //到头了 该循环了
        if (loops > m_uExecutedLoops)
        {
            m_nNextFrame = 0;
            m_uExecutedLoops++;
        }
        //new t for animation
        t = fmodf(t, 1.0);
    }

    CCArray *frames = m_pAnimation->getFrames();
    unsigned int numberOfFrames = frames->count();
    CCSpriteFrame *frameToDisplay = NULL;
    for (unsigned int i=m_nNextFrame;i<numberOfFrames;i++)
    {
        float splitTime = m_pSplitTimes.at(i);

        if (splitTime < t)
        {
            CCAnimationFrame *frame =(CCAnimationFrame*)frames->objectAtIndex(i);
            frameToDisplay = frame->getSpriteFrame();
            ((CCSprite*)m_pTarget)->setDisplayFrame(frameToDisplay);
            m_nNextFrame = i+1;
            break;
        }
    }
}

/*
使动作更连续
 */
void MotionFollowAction::stop(void)
{
    CCNode *tempTarget = m_pTarget;
    if (callBack && target)
    {
        callBack->exec(target);
    }
    if (target)
    {
        target->release();
    }
    info.release();
    if (m_pAnimation->getRestoreOriginalFrame() && m_pTarget)
    {
        ((CCSprite*)m_pTarget)->setDisplayFrame(m_pOrigFrame);
    }
    CCActionInterval::stop();
    if (isTempTarget)
    {
        tempTarget->removeFromParentAndCleanup(true);
    }
    m_pAnimation->release();
}

/****************************************************************************
* FollowAnimationAction 跟随伴随动画动作
*****************************************************************************/
FollowAnimationAction* FollowAnimationAction::create(CCNode *target, float followTime)
{
    FollowAnimationAction *pMoveTo = new FollowAnimationAction();
    pMoveTo->initWithDuration(followTime, ccp(0, 0));
    pMoveTo->autorelease();
    if (target)
    {
        pMoveTo->followTarget = target;
        target->retain();
    }
    return pMoveTo;
}

void FollowAnimationAction::update(float t)
{
    if (followTarget && followTarget->retainCount()>1)
    {
      m_endPosition = followTarget->getPosition();
      m_delta = ccpSub(m_endPosition, m_startPosition);
    }
    if (m_pTarget)
    {
        m_pTarget->setPosition(ccp(m_startPosition.x + m_delta.x * t,
        m_startPosition.y + m_delta.y * t));
    }
}

void FollowAnimationAction::stop(void)
{
    if (callBack && followTarget)
    {
        callBack->exec(followTarget);
    }
    CCNode *tempTarget = m_pTarget;
    if (followTarget)
    {
        followTarget->release();
    }
    CCActionInterval::stop();
    if (isTempTarget)
    {
        tempTarget->removeFromParentAndCleanup(true);
    }
}

void FollowAnimationAction::startWithTarget(CCNode *pTarget)
{
    CCMoveTo::startWithTarget(pTarget);
}

/****************************************************************************
* MotionAction 创建自身动作
*****************************************************************************/
MotionAction* MotionAction::create(const motionInfo& info)
{
    MotionAction *pAnimate = new MotionAction();
    if (pAnimate->init(info))
    {
        pAnimate->autorelease();
    }
    else
    {
        CC_SAFE_RELEASE(pAnimate);
    }
    return pAnimate;
}

MotionAction* MotionAction::create(CCAnimation *animation)
{
    MotionAction *pAnimate = new MotionAction();
    if (pAnimate->init(animation))
    {
        pAnimate->autorelease();
    }
    else
    {
        CC_SAFE_RELEASE(pAnimate);
    }
    return pAnimate;
}

bool MotionAction::init(CCAnimation *animation)
{
    CCAnimation *pAnimate = animation;
    if (!pAnimate) return false;
    initWithAnimation(pAnimate);
    return true;
}

/*
初始化动作
 */
bool MotionAction::init(const motionInfo& info)
{
    motionInfo m = info;
    CCAnimation *pAnimation = m.createAnimation();
    initWithAnimation(pAnimation);
    m.release();
    return true;
}

void MotionAction::stop(void)
{
    CCNode *tempTarget = m_pTarget;
//    CCMoveTo::stop();
    if (isTempTarget)
    {
        tempTarget->removeFromParentAndCleanup(true);
    }
}

/****************************************************************************
* BBMotion 三类动作的包裹类
*****************************************************************************/
/*
自身做动作
目的点 position
 */
void BBMotion::runSelfAction(const motionInfo& info, const CCPoint& position)
{
    if (position.x != 0 && position.y != 0)
    {
        //创建移动动画
        MotionMoveAction *action = MotionMoveAction::create(info, position);
        if (action)
        {
            CCAction *seqAction = CCSequence::create(action,
                      CCCallFunc::create(this, callfunc_selector(BBMotion::doMotionEnd)), NULL);
            if (seqAction)
            {
                sprite->runAction(seqAction);
            }
        }
    }
    else
    {
        //创建自身动画,相当于原地动画
        MotionAction *action = MotionAction::create(info);
        CCFiniteTimeAction *seqAction = CCSequence::create(action,
                CCCallFunc::create(this, callfunc_selector(BBMotion::doMotionEnd)), NULL);
        if (seqAction)
        {
            sprite->runAction(seqAction);
        }
    }
}

/*
附加在离身的动作 指定目的地
 */
void BBMotion::runFlyAction(CCNode *parent, const motionInfo& info, const CCPoint& position)
{
    CCSprite *tempSprite = CCSprite::create();
    parent->addChild(tempSprite);
    if (position.x !=0 && position.y !=0)
    {
        //创建移动动画
        MotionMoveAction *moveAction = MotionMoveAction::create(info, position);
        if (moveAction)
        {
            CCAction *action = CCSequence::create(moveAction,
                    CCCallFuncND::create(this, callfuncND_selector(BBMotion::flyEnd), (void*)tempSprite),NULL);
            tempSprite->runAction(action);
        }
    }
    else
    {
        //创建离身动画,相当于parent不动
        MotionAction *moveAction = MotionAction::create(info);
        if (moveAction)
        {
            CCAction *action = CCSequence::create(moveAction,
                    CCCallFuncND::create(this, callfuncND_selector(BBMotion::flyEnd), (void*)tempSprite),NULL);
            tempSprite->runAction(action);
        }
    }
}

/*
创建跟随动画
碰到对象就消失 指定目的地
 */
void BBMotion::runFollowAction(CCNode *parent, const motionInfo& info, const CCPoint& position)
{
    MotionFollowAction *followAction = MotionFollowAction::create(info, parent);
    if (followAction)
    {
        CCSprite *tempSprite = CCSprite::create();
        parent->addChild(tempSprite);
        CCAction *action = CCSequence::create(followAction,
                CCCallFuncND::create(this, callfuncND_selector(BBMotion::flyEnd), (void*)tempSprite),NULL);

        tempSprite->runAction(action);
    }
}

void BBMotion::runState(CCNode *parent, motionInfo& info, CCNode *target)
{
    CCAnimation *animation = info.createAnimation();
    CCSprite *tempSprite = CCSprite::create();
    parent->addChild(tempSprite);
    tempSprite->runAction(CCRepeatForever::create(CCAnimate::create(animation)));
}

void BBMotion::runMoveState(CCNode *parent, motionInfo& info, CCNode *target, float aniTime)
{
    FollowAnimationAction *follow = FollowAnimationAction::create(target, info.needTime);
    CCSprite *tempSprite = sprite;
    if (info.motionType == motionInfo::OTHER)
    {
        tempSprite = CCSprite::create();
        parent->addChild(tempSprite);
    }
    if (!aniTime) aniTime = info.needTime;
    CCAnimation *animation = info.createAnimation(aniTime);
    tempSprite->setPosition(sprite->getPosition());
    tempSprite->runAction(CCRepeatForever::create(CCAnimate::create(animation)));
    CCAction *action = CCSequence::create(follow,
            CCCallFuncND::create(this, callfuncND_selector(BBMotion::flyEnd), (void*)tempSprite),NULL);

    tempSprite->runAction(action);
}

void BBMotion::runMoveAction(CCNode *parent, motionInfo& info, CCNode *target, float animationTime, const CCPoint& point)
{
    CCMoveTo *moveTo = CCMoveTo::create(info.needTime,point);
    CCSprite *tempSprite = sprite;

    if (!animationTime) animationTime = info.needTime;
    int step = info.needTime / animationTime;
    animationTime = info.needTime / step;
    CCAnimation * animation = info.createAnimation(animationTime);
    CCFiniteTimeAction *action = CCSpawn::create(CCRepeat::create(CCAnimate::create(animation),step),moveTo,NULL);
    CCFiniteTimeAction *timeAction = CCSequence::create(action,
            CCCallFunc::create(this, callfunc_selector(BBMotion::doMotionEnd)),NULL);
    tempSprite->runAction(timeAction);
}

/*
组合动作,新式动作
@param parent fly 承载节点
@param info 动画信息
@param point 目的地
@param target 跟随对象
i.e : motionInfo::create()->addNextMotionInfo(motionInfo::TOGETHER, info1)->addNextMotionInfo(motionInfo::TOGETHER, info2)
 */

void BBMotion::runAction(CCNode *parent, const motionInfo* info, CCPoint& position, CCNode *target, TargetCallBack *callBack)
{
    if(!info) return;
    motionInfo *root = (motionInfo*)info;
    int nextCombine = root->nextCombineType;
    CCFiniteTimeAction *preAction = NULL;
    CCFiniteTimeAction *nowAction = NULL;
    while (root)
    {
        nowAction = root->createAction(sprite, parent, position, target, callBack);
        if (!preAction)
        {
            preAction = CCSequence::create(nowAction,NULL);
        }
        else
        {
            switch (nextCombine)
            {
                case motionInfo::SINGLE:
                {
                    preAction = CCSequence::create(preAction,nowAction,NULL);
                }
                    break;
                case motionInfo::TOGETHER:
                {
                    preAction = CCSpawn::create(preAction,nowAction,NULL);
                }
                    break;
            }
        }
        nextCombine = root->nextCombineType;
        root = root->nextMotion;
    }
    if (preAction)
    {
        CCFiniteTimeAction *seqaction = CCSequence::create(preAction,/*CCDelayTime::create(0.5),*/
                CCCallFunc::create(this, callfunc_selector(BBMotion::doMotionEnd)),NULL);

        if (seqaction)
        {
            seqaction->setTag(info->actionTag);
            sprite->runAction(seqaction);
        }
    }
}

void BBMotion::flyEnd(CCNode *pSender, void *arg)
{
    if (pSender)
    {
        pSender->removeFromParentAndCleanup(true);
    }
    doMotionEnd();
}

void BBMotion::doMotionEnd()
{
    nextStep();
    this->putCombineAction();
}

/*
通过id确定具体的某个初始执行
 */
void BBMotion::start(int id)
{
    tryNextAction(id);
    motionInfo *info = makeMotion(id);
    if (info)
    {
        putMotion(info);
    }
}

void BBMotion::tryNextAction(int id)
{
    actionType = id;
}

/*
或运算 实现并行的执行动作
 */
void BBMotion::tryCombineAction(int type)
{
    actionType = actionType | type;
}

bool BBMotion::isNowAction(int actionType)
{
    if (oldActionType == -1) return false;
    return oldActionType & actionType;
}

/*
下一个动作去除 该系
 */
void BBMotion::clearAction(int type)
{
    /*
    可能有错 主要是赋予actionType生成一个新的常数
     */
    actionType &= ~type;
}

void BBMotion::putCombineAction()
{
    for(int i=0;i<16;i++)
    {
        if(actionType & (1<<i))
        {
            motionInfo *motionInfo = makeMotion(1<<i);
            if (motionInfo)
            {
                clearAction(1<<i);
                putMotion(motionInfo);
                return ;
            }
        }
    }
    motionInfo *motionInfo = makeMotion(1<<5);
    if (motionInfo)
    {
        clearAction(1<<5);
        putMotion(motionInfo);
        return ;
    }
}

void BBMotion::setAction(int actionType)
{
    oldActionType = actionType;
}

motionInfo* BBMotion::makeMotion(int actionType)
{
    oldActionType = actionType;
    return v_makeMotion(actionType);
}

void BBMotion::putMotion(motionInfo *info)
{
    v_putMotion(info);
}

bool MotionDirAction::getFrames(std::vector<CCSpriteFrame *>& frames)
{
    for (std::vector<stFrameInfo>::iterator iter = this->frames.begin(); iter != this->frames.end();++iter)
    {

        CCTexture2D *texture = CCTextureCache::sharedTextureCache()->addImage(iter->frameName.c_str());
        CCSpriteFrame *frame = iter->frame;
        if (texture && !iter->frame)
        {
            frame = CCSpriteFrame::createWithTexture(texture,CCRectMake(0,0,texture->getContentSize().width,texture->getContentSize().height));
            frame->retain();
            frame->setOffset(iter->offset);
            iter->frame = frame;
        }
        if (frame)
        {
            frame->retain();
            frames.push_back(frame);
        }
    }
    return true;
}

void MotionDirAction::takeNode(script::xmlCodeNode *node)
{
    frameLoadType = node->getAttr("framestype");
    FramesName = node->getAttr("framenode");

    script::xmlCodeNode frameNode = node->getFirstChild("frame");
    while(frameNode.isValid())
    {
        stFrameInfo info;
        info.frameName = frameNode.getAttr("name");
        info.offset.x = frameNode.getInt("offsetx");
        info.offset.y = frameNode.getInt("offsety");
        frames.push_back(info);
        frameNode = frameNode.getNextChild("frame");
    }
}

void MotionConbineAction::takeNode(script::xmlCodeNode *node)
{
    MotionXmlAction::takeNode(node);
    std::string nextTypeStr = node->getAttr("nexttype");
    if (nextTypeStr == "conbine")
    {
        nextType = 1;
    }
    else
        nextType = 0;
}

motionInfo* MotionConbineAction::getMotionInfo(int dir)
{
    motionInfo *info = motionInfo::create();
    if (getFrames(dir, info->frames))
    {
        info->actionType = actionType;
        info->motionName = actionName;
        info->needTime = needTime;
        info->frameType = frameType;
        info->nextCombineType = this->nextType;
        info->motionType = motionType;
    }
    return info;
}

/*
处理节点
 */
void MotionXmlAction::takeNode(script::xmlCodeNode *node)
{
    actionName = node->getAttr("name");
    node->getAttr("type",actionType);
    std::string frameTypeStr = node->getAttr("frametype");
    if (frameTypeStr == "follow")
    {
        this->frameType = motionInfo::FOLLOW_FRAMES;
    }
    else if (frameTypeStr == "times")
    {
        this->frameType = motionInfo::TIME_FRAMES;
    }
    else if (frameTypeStr == "followanimations")
    {
        this->frameType = motionInfo::FOLLOW_ANIMATIONS;
    }
    else
    {
        this->frameType = motionInfo::MOVE_FRAMES;
    }
    std::string motionTypeStr = node->getAttr("motiontype");
    if (motionTypeStr == "other")
    {
        this->motionType = motionInfo::OTHER;
    }
    else
    {
        this->motionType = motionInfo::SELF;
    }
    script::xmlCodeNode dirNode = node->getFirstChild("dir");
    if(dirNode.isValid())
    {
        int dirId = 0;
        MotionDirAction dirAction;
        dirNode.getAttr("id",dirId);
        dirAction.takeNode(&dirNode);
        dirActions[dirId] = dirAction;
    }
    node->getAttr("needtime",needTime);
    node->getAttr("actiontype",actionType);
}

/**
 * 获取帧集合
 * \param frames 帧集合
 */
bool MotionXmlAction::getFrames(int dir,std::vector<CCSpriteFrame *> &frames)
{
    DIRACTIONS_ITER iter = dirActions.find(dir);
    if (iter != dirActions.end())
    {
        if (iter->second.frameLoadType == "frames")
        {
            return iter->second.getFrames(frames);
        }
        if (iter->second.frameLoadType == "pack")
        {
            std::string &framesName = iter->second.FramesName;
            //	return PngPack::getMe().getFrames(framesName.c_str(),frames);
        }
    }
    return false;
}