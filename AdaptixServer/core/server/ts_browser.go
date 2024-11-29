package server

import (
	"AdaptixServer/core/utils/krypt"
	"bytes"
	"encoding/json"
	"fmt"
	"time"
)

/// AGENT

func (ts *Teamserver) TsAgentBrowserDisks(agentId string, username string) error {
	var (
		err         error
		agentObject bytes.Buffer
		agent       *Agent
		taskData    TaskData
		data        []byte
	)

	value, ok := ts.agents.Get(agentId)
	if ok {

		agent, _ = value.(*Agent)
		_ = json.NewEncoder(&agentObject).Encode(agent.Data)

		data, err = ts.Extender.ExAgentBrowserDisks(agent.Data.Name, agentObject.Bytes())
		if err != nil {
			return err
		}

		err = json.Unmarshal(data, &taskData)
		if err != nil {
			return err
		}

		if taskData.TaskId == "" {
			taskData.TaskId, _ = krypt.GenerateUID(8)
		}
		taskData.AgentId = agentId
		taskData.User = username
		taskData.StartDate = time.Now().Unix()

		agent.TasksQueue.Put(taskData)

	} else {
		return fmt.Errorf("agent '%v' does not exist", agentId)
	}

	return nil
}

/// SYNC

func (ts *Teamserver) TsClientBrowserDisks(jsonTask string, jsonDrives string) {
	var (
		agent    *Agent
		task     TaskData
		taskData TaskData
		value    any
		ok       bool
		err      error
	)

	err = json.Unmarshal([]byte(jsonTask), &taskData)
	if err != nil {
		return
	}

	value, ok = ts.agents.Get(taskData.AgentId)
	if ok {
		agent = value.(*Agent)
	} else {
		return
	}

	value, ok = agent.Tasks.Get(taskData.TaskId)
	if ok {
		task = value.(TaskData)
	} else {
		return
	}

	if task.Type != TYPE_BROWSER {
		return
	}

	agent.Tasks.Delete(taskData.TaskId)

	if taskData.MessageType != CONSOLE_OUT_ERROR && taskData.MessageType != CONSOLE_OUT_LOCAL_ERROR {
		taskData.Message = "Status: OK"
	}

	packet := CreateSpBrowserDisks(taskData, jsonDrives)
	ts.TsSyncClient(task.User, packet)
}
