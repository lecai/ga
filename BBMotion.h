#include "CCPlatformMacros.h"//
// Created by eacan  on 13-9-29 下午1:31.
//
// To change the template use AppCode | Preferences | File Templates.
//



#ifndef __BBMotion_H_
#define __BBMotion_H_

#include <iostream>
#include "cocos2d.h"
#include "BBXmlScript.h"
#include "CCNode.h"
#include "CCSprite.h"

USING_NS_CC;

/*
 动画 移动到 target 时候 回调函数
 */
class TargetCallBack
{
public:
    virtual void exec(CCNode *node) = 0;
};

class motionInfo :public CCObject
{
public:
    static motionInfo *create(const motionInfo &info);
    static motionInfo *create();

    /*
     根据自身创建相应的action
     param parent 离身的node(ccsprite)
     */
    CCFiniteTimeAction *createAction(CCNode *self,CCNode *parent, const CCPoint &point,CCNode *target,TargetCallBack *callback);

    int actionTag;
    int motionId;  //动画编号
    std::string motionName; //动画名称
    enum
    {
        SELF = 0, //自身
        OTHER = 1,//离身
        FOLLOW = 2,//跟随
    };
    int motionType; //动画类型

    enum
    {
        TIME_FRAMES = 0,
        MOVE_FRAMES = 1,
        FOLLOW_FRAMES = 2,
        FOLLOW_ANIMATIONS =3,
    };
    int frameType;//帧类型

    float needTime;//所需时间
    int actionType;//动作类型,子类定义
    std::vector<CCSpriteFrame *> frames; //临时变量
    int nextCombineType;//下一个合并类型

    int delayTime;//延时时间

    enum
    {
        SINGLE, //单个序列
        TOGETHER,//一起
    };
    //控制动画播放的概率
    bool isValid(){return false;}

    motionInfo *addNextMotionInfo(int nextCombineType, const motionInfo &info);
    motionInfo *nextMotion;

    //释放此motion
    void release();

    /*
    创建动画帧
     */
    CCAnimation *createAnimation(float time = 0);

    motionInfo()
    {
        nextCombineType = SINGLE;
        motionId = actionType = actionTag = motionType = needTime = -1;
        nextMotion = NULL;
        frameType = MOVE_FRAMES;
        delayTime = 0;
    }

    motionInfo &operator = (const motionInfo &info)
    {
        this->nextCombineType = info.nextCombineType;
        this->motionId = info.motionId;
        this->actionType = info.actionType;
        this->actionTag = info.actionTag;
        this->motionType = info.motionType;
        this->needTime=info.needTime;
        this->frameType = info.frameType;
        this->delayTime = info.delayTime;
        return *this;
    }
};

/*
动画移动动作,作用于自身
动画帧与移动相关
 */
class MotionMoveAction:public CCMoveTo
{
public:
    virtual void update(float time);
    bool isTempTarget;
    MotionMoveAction()
    {
        isTempTarget = false;
    }

    /*
    创建动画
     */
    static MotionMoveAction *create(const motionInfo &info, const CCPoint &position);

    /*
    初始化motionInfo 移动行为
     */
    bool init(const motionInfo &info, const CCPoint &position);

    /*
    释放移动 所占有的帧
     */
    void stop(void);

protected:
    motionInfo info;
    CCPoint m_delta;
};

/*
创建非移动的自身动画
 */
class MotionAction :public CCAnimate
{
public:
    bool isTempTarget;
    MotionAction()
    {
        isTempTarget = false;
    }
    virtual void update(float time)
    {
        CCAnimate::update(time);
    }
    static MotionAction *create(CCAnimation *animation);
    bool init(CCAnimation *animation);

    /*
    创建motionInfo动画
     */
    static MotionAction *create(const motionInfo &info);

    /*
    初始化动作
     */
    bool init(const motionInfo &info);

    /*
    释放自己 所占有的帧
     */
    void stop(void);
};

/*
特殊的动作
跟随目标对象来动画
自身动画不关联
 */
class MotionFollowAction :public CCMoveTo
{
public:
    bool isTempTarget;
    TargetCallBack *callBack;
    MotionFollowAction()
    {
        isTempTarget = false;
        target = NULL;
        callBack = NULL;
    }
    static MotionFollowAction *create(const motionInfo &info,CCNode *target);
    /*
    更新
     */
    virtual void update(float t);
    void stop(void);

    bool initWithAnimation(CCAnimation *pAnimation);

    virtual void startWithTarget(CCNode *pTarget);
    void updateAnimation(float t);

private:
    CCNode *target;
    motionInfo info;
    /*
    animate数据
     */
    CCAnimation *m_pAnimation;
    std::vector<float> m_pSplitTimes;
    int m_nNextFrame;
    CCSpriteFrame *m_pOrigFrame;
    unsigned int m_uExecutedLoops;
    CCPoint m_delta;
};

class FollowAnimationAction:public CCMoveTo
{
public:
    FollowAnimationAction()
    {
        isTempTarget = false;
        followTarget = NULL;
        callBack = NULL;
    }
    static FollowAnimationAction *create(CCNode *target, float followTime);

    virtual void update(float t);

    void stop(void);
    virtual void startWithTarget(CCNode *pTarget);

    bool isTempTarget ;
    TargetCallBack *callBack;
private:
    CCNode *followTarget; //follow me
    CCPoint m_delta;
};

/*
操作类(包裹类)
 */
class BBMotion:public CCSprite
{
public:
    /*
    自身动作
    @param position 目标点

     */
    void runSelfAction(const motionInfo &info, const CCPoint &position);

    /*
    附加在离身的动作 指定目的地
     */
    void runFlyAction(CCNode *parent, const motionInfo &info, const CCPoint &position);

    /*
    根据动作
    碰到对象就消失
     */
    void runFollowAction(CCNode *parent, const motionInfo &info, const CCPoint &position);
    void runState(CCNode *parent,motionInfo &info,CCNode *target);
    void runMoveState(CCNode *parent,motionInfo &info,CCNode *target, float aniTime=0);
    void runMoveAction(CCNode *parent,motionInfo &info,CCNode *target, float animationTime,const CCPoint &point);


    /*
    组合动画播放
    @param parent 承载node
    @param info 动画信息
    @param position 目的地
    @param target 目标
     */
    void runAction(CCNode *parent, const motionInfo *info,CCPoint &position,CCNode *target,TargetCallBack *callBack = NULL);


    /*
    飞行结束后 清除临时节点
     */
    void flyEnd(CCNode *pSender,void *arg);

    /*
    动画结束
     */
    void doMotionEnd();

    /*
    执行下一步
     */
    virtual void nextStep(){}

    void attachSprite(CCSprite *sprite)
    {
        this->sprite = sprite;
    }

private:
    CCSprite *sprite;
public:
    /*
    初始化状态
    @param id 对应着某个行为
     */
    void start(int id);
    void tryNextAction(int id);
    /*

     */
    void tryCombineAction(int type);
    int actionType;
    int oldActionType;
    BBMotion()
    {
        actionType = 0;
        oldActionType = -1;
        sprite = NULL;
    }
    /*
    检查当前行为
     */
    bool isNowAction(int actionType);

    /*
    下个行为去除此行为
     */
    void clearAction(int type);
    /*
    根据优先级 生成相应的文件
     */
    void putCombineAction();

protected:
    void setAction(int actionType);
    virtual motionInfo *makeMotion(int actionType);
    virtual void putMotion(motionInfo *info);
    virtual motionInfo *v_makeMotion(int actionType){return NULL;}
    virtual void v_putMotion(motionInfo *info){}
};

struct stFrameInfo{
    std::string frameName;
    CCPoint offset;
    CCSpriteFrame *frame;
    stFrameInfo()
    {
        frame = NULL;
    }
};
class MotionDirAction{
public:
    std::string FramesName; // 帧名字
    std::string frameLoadType; // 帧加载类型
    std::vector<stFrameInfo> frames;
    MotionDirAction()
    {

    }
    ~MotionDirAction()
    {
        for (std::vector<stFrameInfo>::iterator iter = frames.begin(); iter != frames.end();++iter)
        {
            if (iter->frame) iter->frame->release();
        }
    }
    /**
     * 获取多帧
     */
    bool getFrames(std::vector<CCSpriteFrame *> &frames);
    /**
     * 处理节点
     */
    void takeNode(script::xmlCodeNode *node);
};

class MotionXmlAction{
public:
    std::string actionName; // 动作名字
    int actionType; // 动作类型
    float needTime; // 需要时间
    int frameType; // 帧类型 时间 或者移动类型
    int motionType; // 动画类型 [SELF,OTHER]
    std::map<int,MotionDirAction> dirActions;
    typedef std::map<int,MotionDirAction>::iterator DIRACTIONS_ITER;
    /**
     * 获取帧集合
     * \param frames 帧集合
     */
    bool getFrames(int dir,std::vector<CCSpriteFrame *> &frames);
    MotionXmlAction()
    {
        actionType = 0;
        needTime = 0;
        motionType = 0;
        frameType = 0;
    }
    /**
     * 处理节点
     */
    void takeNode(script::xmlCodeNode *node);
};
/**
 * 组合拳
 */
class MotionConbineAction:public MotionXmlAction{
public:
    int nextType; // 下一个类型
    /**
     * 处理节点
     */
    void takeNode(script::xmlCodeNode *node);
    /**
     * 构建动画
     */
    motionInfo* getMotionInfo(int dir);
};
#endif //__BBMotion_H_
