#include "GitConfig.h"

#include <GitBase.h>
#include <GitCloneProcess.h>
#include <QLogger.h>

using namespace QLogger;

bool GitUserInfo::isValid() const
{
   return !mUserEmail.isNull() && !mUserEmail.isEmpty() && !mUserName.isNull() && !mUserName.isEmpty();
}

GitConfig::GitConfig(QSharedPointer<GitBase> gitBase, QObject *parent)
   : QObject(parent)
   , mGitBase(gitBase)
{
}

GitUserInfo GitConfig::getGlobalUserInfo() const
{
   GitUserInfo userInfo;

   QLog_Debug("Git", QString("Getting global user info"));

   const auto nameRequest = mGitBase->run("git config --get --global user.name");

   if (nameRequest.success)
      userInfo.mUserName = nameRequest.output.toString().trimmed();

   const auto emailRequest = mGitBase->run("git config --get --global user.email");

   if (emailRequest.success)
      userInfo.mUserEmail = emailRequest.output.toString().trimmed();

   return userInfo;
}

void GitConfig::setGlobalUserInfo(const GitUserInfo &info)
{
   QLog_Debug("Git", QString("Setting global user info"));

   mGitBase->run(QString("git config --global user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --global user.email %1").arg(info.mUserEmail));
}

GitExecResult GitConfig::setGlobalData(const QString &key, const QString &value)
{
   QLog_Debug("Git", QString("Configuring global key {%1} with value {%2}").arg(key, value));

   return mGitBase->run(QString("git config --global %1 \"%2\"").arg(key, value));
}

GitUserInfo GitConfig::getLocalUserInfo() const
{
   QLog_Debug("Git", QString("Getting local user info"));

   GitUserInfo userInfo;

   const auto nameRequest = mGitBase->run("git config --get --local user.name");

   if (nameRequest.success)
      userInfo.mUserName = nameRequest.output.toString().trimmed();

   const auto emailRequest = mGitBase->run("git config --get --local user.email");

   if (emailRequest.success)
      userInfo.mUserEmail = emailRequest.output.toString().trimmed();

   return userInfo;
}

void GitConfig::setLocalUserInfo(const GitUserInfo &info)
{
   QLog_Debug("Git", QString("Setting local user info"));

   mGitBase->run(QString("git config --local user.name \"%1\"").arg(info.mUserName));
   mGitBase->run(QString("git config --local user.email %1").arg(info.mUserEmail));
}

GitExecResult GitConfig::setLocalData(const QString &key, const QString &value)
{

   QLog_Debug("Git", QString("Configuring local key {%1} with value {%2}").arg(key, value));

   return mGitBase->run(QString("git config --local %1 \"%2\"").arg(key, value));
}

GitExecResult GitConfig::clone(const QString &url, const QString &fullPath)
{
   QLog_Debug("Git", QString("Starting the clone process for repo {%1} at {%2}.").arg(url, fullPath));

   const auto asyncRun = new GitCloneProcess(mGitBase->getWorkingDir());
   connect(asyncRun, &GitCloneProcess::signalProgress, this, &GitConfig::signalCloningProgress, Qt::DirectConnection);

   mGitBase->setWorkingDir(fullPath);

   return asyncRun->run(QString("git clone --progress %1 %2").arg(url, fullPath));
}

GitExecResult GitConfig::initRepo(const QString &fullPath)
{
   QLog_Debug("Git", QString("Initializing a new repository at {%1}").arg(fullPath));

   const auto ret = mGitBase->run(QString("git init %1").arg(fullPath));

   if (ret.success)
      mGitBase->setWorkingDir(fullPath);

   return ret;
}

GitExecResult GitConfig::getLocalConfig() const
{
   QLog_Debug("Git", QString("Getting local config"));

   return mGitBase->run("git config --local --list");
}

GitExecResult GitConfig::getGlobalConfig() const
{
   QLog_Debug("Git", QString("Getting global config"));

   return mGitBase->run("git config --global --list");
}

GitExecResult GitConfig::getRemoteForBranch(const QString &branch)
{
   QLog_Debug("Git", QString("Getting remote for branch {%1}.").arg(branch));

   const auto config = getLocalConfig();

   if (config.success)
   {
      const auto values = config.output.toString().split('\n', QString::SkipEmptyParts);
      const auto configKey = QString("branch.%1.remote=").arg(branch);
      QString configValue;

      for (const auto &value : values)
      {
         if (value.startsWith(configKey))
         {
            configValue = value.split("=").last();
            break;
         }
      }

      if (!configValue.isEmpty())
         return { true, configValue };
   }

   return GitExecResult();
}
