<?php

/*
  +------------------------------------------------------------------------+
  | Phalcon Framework                                                      |
  +------------------------------------------------------------------------+
  | Copyright (c) 2011-2012 Phalcon Team (http://www.phalconphp.com)       |
  +------------------------------------------------------------------------+
  | This source file is subject to the New BSD License that is bundled     |
  | with this package in the file docs/LICENSE.txt.                        |
  |                                                                        |
  | If you did not receive a copy of the license and are unable to         |
  | obtain it through the world-wide-web, please send an email             |
  | to license@phalconphp.com so we can send you a copy immediately.       |
  +------------------------------------------------------------------------+
  | Authors: Andres Gutierrez <andres@phalconphp.com>                      |
  |          Eduar Carvajal <eduar@phalconphp.com>                         |
  +------------------------------------------------------------------------+
*/

class DispatcherMvcTest extends PHPUnit\Framework\TestCase
{

	public function dispatcherAutoloader($className)
	{
		if (file_exists('unit-tests/controllers/'.$className.'.php')) {
			require 'unit-tests/controllers/'.$className.'.php';
		}
		if (file_exists('unit-tests/logics/'.$className.'.php')) {
			require 'unit-tests/logics/'.$className.'.php';
		}
	}

	public function setUp()
	{
		spl_autoload_register(array($this, 'dispatcherAutoloader'));
	}

	public function tearDown()
	{
		spl_autoload_unregister(array($this, 'dispatcherAutoloader'));
	}

	public function testDispatcher()
	{

		Phalcon\Di::reset();

		$di = new Phalcon\Di();

		$di->set('response', new \Phalcon\Http\Response());

		$dispatcher = new Phalcon\Mvc\Dispatcher();
		$dispatcher->setDI($di);
		$this->assertInstanceOf('Phalcon\Di', $dispatcher->getDI());

		$di->set('dispatcher', $dispatcher);

		$dispatcher->setControllerName('index');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());

		try {
			$dispatcher->dispatch();
			$this->assertTrue(FALSE, 'oh, Why?');
		}
		catch(Phalcon\Exception $e){
			$this->assertEquals($e->getMessage(), "IndexController handler class cannot be loaded");
			$this->assertInstanceOf('Phalcon\Mvc\Dispatcher\Exception', $e);
		}

		$dispatcher->setControllerName('essai');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());

		try {
			$dispatcher->dispatch();
			$this->assertTrue(FALSE, 'oh, Why?');
		}
		catch(Phalcon\Exception $e){
			$this->assertEquals($e->getMessage(), "EssaiController handler class cannot be loaded");
			$this->assertInstanceOf('Phalcon\Mvc\Dispatcher\Exception', $e);
		}

		$dispatcher->setControllerName('test0');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());

		try {
			$dispatcher->dispatch();
			$this->assertTrue(FALSE, 'oh, Why?');
		}
		catch(Phalcon\Exception $e){
			$this->assertEquals($e->getMessage(), "Test0Controller handler class cannot be loaded");
			$this->assertInstanceOf('Phalcon\Mvc\Dispatcher\Exception', $e);
		}

		$dispatcher->setControllerName('test1');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());

		try {
			$dispatcher->dispatch();
			$this->assertTrue(FALSE, 'oh, Why?');
		}
		catch(Phalcon\Exception $e){
			$this->assertEquals($e->getMessage(), "Action 'index' was not found on handler 'test1'");
		}

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());
		$controller = $dispatcher->dispatch();
		$this->assertInstanceOf('Test2Controller', $controller);

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('essai');
		$dispatcher->setParams(array());

		try {
			$dispatcher->dispatch();
			$this->assertTrue(FALSE, 'oh, Why?');
		}
		catch(Phalcon\Exception $e){
			$this->assertEquals($e->getMessage(), "Action 'essai' was not found on handler 'test2'");
		}

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('other');
		$dispatcher->setParams(array());
		$controller = $dispatcher->dispatch();
		$this->assertInstanceOf('Test2Controller', $controller);

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('another');
		$dispatcher->setParams(array());
		$dispatcher->dispatch();
		$value = $dispatcher->getReturnedValue();
		$this->assertEquals($value, 100);

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('anotherTwo');
		$dispatcher->setParams(array(2, "3"));
		$dispatcher->dispatch();
		$value = $dispatcher->getReturnedValue();
		$this->assertEquals($value, 5);

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('anotherthree');
		$dispatcher->setParams(array());
		$dispatcher->dispatch();
		$value = $dispatcher->getActionName();
		$this->assertEquals($value, 'anotherfour');
		$value = $dispatcher->getReturnedValue();
		$this->assertEquals($value, 120);

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('anotherFive');
		$dispatcher->setParams(array("param1" => 2, "param2" => 3));
		$dispatcher->dispatch();
		$value = $dispatcher->getReturnedValue();
		$this->assertEquals($value, 5);

		$dispatcher->setControllerName('test7');
		$dispatcher->setActionName('service');
		$dispatcher->setParams(array());
		$dispatcher->dispatch();
		$value = $dispatcher->getReturnedValue();
		$this->assertEquals($value, "hello");

		$dispatcher->setControllerName('nofound');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());
		$dispatcher->setErrorHandler('Error::index', Phalcon\Dispatcher::EXCEPTION_HANDLER_NOT_FOUND);

		try {
			$dispatcher->dispatch();
		}
		catch(Phalcon\Exception $e){
			$this->assertEquals($e->getMessage(), "ErrorController handler class cannot be loaded");
		}
	}

	public function testDispatcherForward()
	{
		Phalcon\Di::reset();

		$di = new Phalcon\Di();

		//$di->set('response', new \Phalcon\Http\Response());

		$dispatcher = new Phalcon\Mvc\Dispatcher();
		$dispatcher->setDI($di);

		$di->set('dispatcher', $dispatcher);

		$dispatcher->setControllerName('test2');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());

		$dispatcher->forward(array('controller' => 'test3', 'action' => 'other'));

		$value = $dispatcher->getControllerName();
		$this->assertEquals($value, 'test3');

		$value = $dispatcher->getActionName();
		$this->assertEquals($value, 'other');

		$value = $dispatcher->getPreviousControllerName();
		$this->assertEquals($value, 'test2');

		$value = $dispatcher->getPreviousActionName();
		$this->assertEquals($value, 'index');
	}

	public function testIssues2270()
	{
		Phalcon\Di::reset();

		$di = new \Phalcon\Di();

		$dispatcher = new \Phalcon\Mvc\Dispatcher();
		$dispatcher->setDI($di);

		$dispatcher->setDefaultNamespace('A\B\C');
		$dispatcher->setControllerName('Test');
		$dispatcher->setActionName('index');

		$this->assertEquals('A\B\C\TestController', $dispatcher->getHandlerClass());
	}

	public function testBindLogic()
	{
		Phalcon\Di::reset();

		$di = new \Phalcon\Di();

		$dispatcher = new \Phalcon\Mvc\Dispatcher();
		$dispatcher->setDI($di);
		$dispatcher->setLogicBinding(true);

		$dispatcher->setControllerName('Logic');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array("param1" => 2, "param2" => 3));
		$dispatcher->dispatch();

		$handle = $dispatcher->getActiveHandler();
		$this->assertEquals(get_class($handle), 'LogicController');

		$value = $dispatcher->getReturnedValue();
		$this->assertEquals(get_class($value), 'MyLogic');

		$this->assertEquals($value->num, 2);
		$this->assertEquals($value->param1, 2);
		$this->assertEquals($value->param2, 3);

		$this->assertEquals($value->getActionParams(), array("param1" => 2, "param2" => 3));
		$this->assertEquals($value->getActionName(), 'index');
		$this->assertEquals($dispatcher->getParams(), array("param1" => 2, "param2" => 3));
		$this->assertEquals($dispatcher->getParam('param1'), 2);
		$this->assertEquals($dispatcher->getParam('param2'), 3);
	}

	public function testDispatcherContinue()
	{
		Phalcon\Di::reset();

		$di = new Phalcon\Di();

		$dispatcher = new Phalcon\Mvc\Dispatcher();
		$dispatcher->setDI($di);

		$di->set('dispatcher', $dispatcher);

		$dispatcher->setControllerName('continue');
		$dispatcher->setActionName('index');
		$dispatcher->setParams(array());

		$dispatcher->dispatch();

		$value = $dispatcher->getReturnedValue();
		$this->assertEquals($value, 'ok');

		$error = $dispatcher->getLastException();
		$this->assertEquals(get_class($error), 'Phalcon\ContinueException');
	}

}
